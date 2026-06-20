const { createApp } = Vue;

// Non-reactive state (WebSocket, Web Audio objects, queues, timers).
// Kept out of Vue's reactive data on purpose: wrapping Web Audio / WebSocket
// objects in a reactive proxy is unnecessary and can cause subtle issues.
const state = {
    ws: null,
    audioContext: null,
    audioProcess: null,
    source: null,
    audioBip: null,
    audioRing: null,
    recvQueue: [],
    sendQueue: [],
    timeout: null,          // websocket life timeout
    reconnectTimeout: null,
    videoReloadTimeout: null,
    frameBySample: 0,
    rate: 0,
};

const app = createApp({
    data() {
        return {
            message: {
                color: "white",
                text: "starting",
            },
            videoSrc: "",
            mbModeClient: false,
            listening: false,
            recording: false,
            password: null,
            bPasswordOk: false,
            bPasswordSaved: true,
            bAudioBusy: false,
            devices: [],
            deviceId: null,
            ringing: false,
        };
    },

    computed: {
        // Map the status message colour onto the status-pill style classes
        statusClass() {
            if ("green" == this.message.color) return "ok";
            if ("red" == this.message.color) return "bad";
            return "";
        },
    },

    created() {
        // Useful in server mode, generate video URL
        const colonIndex = window.location.host.lastIndexOf(':');
        if (colonIndex > 0) {
            this.videoSrc = `http://${window.location.host.substr(0, colonIndex)}:8081`;
        }
        else {
            this.videoSrc = `http://${window.location.host}:8081`;
        }

        // Read configuration and then start websocket if needed
        fetch("/config").then((response) => response.json()).then(
            (config) => {
                console.log(config);
                if (config) {
                    this.mbModeClient = ("client" == config.mode);
                    state.frameBySample = config.frameBySample;
                    state.rate = config.rate;
                    if (config.videoSrc) {
                        this.videoSrc = config.videoSrc;
                    }
                    if (!this.mbModeClient) {
                        // Start websocket
                        state.ws = this.connect();
                    }
                    else {
                        this.message.text = "";
                    }
                }
            }
        ).catch(() => {
            this.message.text = "Failed to read configuration";
            this.message.color = "red";
        });

        // List available "input devices". Useful for chrome but not for firefox.
        // navigator.mediaDevices is undefined in insecure (non-https) contexts.
        if (navigator.mediaDevices && navigator.mediaDevices.enumerateDevices) {
            navigator.mediaDevices.enumerateDevices().then((devices) => {
                devices.forEach((device) => {
                    if (device.kind == "audioinput" && device.deviceId != "default") {
                        this.devices.push(device);
                    }
                });
                console.log("devices:", this.devices);
                // Read device ID in localStorage
                this.deviceId = localStorage.getItem("deviceId");
                console.log("this.deviceId:", this.deviceId);
            });
        }
        else {
            this.deviceId = localStorage.getItem("deviceId");
        }

        // At startup, load saved password
        this.password = localStorage.getItem("password");
        this.bPasswordSaved = true;
        this.sendPassword(false);
    },

    methods: {
        // Simple http get function without waiting for result
        get(msg) {
            fetch(msg).then(
                () => console.log(`get ${msg} OK`),
                () => console.log(`get ${msg} failed`)
            );
        },

        // Websocket
        onopen() {
            console.log('websocket onopen');
            this.message.text = "Connected to server";
            this.message.color = "green";
        },
        onclose() {
            console.log('websocket onclose');
            this.message.text = "Disconnected from server";
            this.message.color = "red";
            this.hangup(false);
            // Try to reconnect in 5 seconds
            state.reconnectTimeout = setTimeout(() => {
                state.ws = this.connect();
            }, 5000);
        },
        onmessage(msg) {
            if ("string" == typeof msg.data) {
                if ("doorbell" == msg.data) {
                    this.ring();
                }
                else if ("audiobusy" == msg.data || "audiofree" == msg.data) {
                    this.checkAudioBusy();
                }
                else if ("alive" == msg.data) {
                    // TODO ADD life message from client to server
                    this.relaunchTimeout();
                }
            }
            else if (this.listening) {
                const fileReader = new FileReader();
                // onloadend will be called once readAsArrayBuffer has finished
                fileReader.onloadend = () => {
                    // Convert from int to float
                    let sampleIntArray = new Int16Array(fileReader.result);
                    let sampleFloatArray = new Float32Array(sampleIntArray.length);
                    for (let i = 0; i < sampleIntArray.length; ++i) {
                        sampleFloatArray[i] = sampleIntArray[i] / 0x7FFF;
                    }
                    // If rate of audio context is not the same of sample, convert it to expected sample rate
                    if (state.audioContext.sampleRate != state.rate) {
                        const offlineCtx = new OfflineAudioContext(1, state.frameBySample * (state.audioContext.sampleRate / state.rate), state.audioContext.sampleRate);
                        const source = offlineCtx.createBufferSource();
                        source.buffer = state.audioContext.createBuffer(1, state.frameBySample, state.rate);
                        for (let i = 0; i < sampleFloatArray.length; ++i) {
                            source.buffer.getChannelData(0)[i] = sampleFloatArray[i];
                        }
                        source.connect(offlineCtx.destination);
                        source.start();
                        offlineCtx.startRendering().then((renderedBuffer) => {
                            for (let i = 0; i < renderedBuffer.length; ++i) {
                                state.recvQueue.push(renderedBuffer.getChannelData(0)[i]);
                            }
                        }).catch((err) => {
                            console.log('Rendering failed: ' + err);
                        });
                    }
                    else {
                        // No need to convert sample rate
                        for (let i = 0; i < sampleFloatArray.length; ++i) {
                            state.recvQueue.push(sampleFloatArray[i]);
                        }
                    }
                };
                fileReader.readAsArrayBuffer(msg.data);
            }
        },
        onerror() {
            console.log('websocket onerror');
        },
        connect() {
            const wsurl = window.location.origin.replace("http", "ws");
            console.log('connect to ', wsurl);
            const w = new WebSocket(wsurl);
            w.onopen = this.onopen;
            w.onclose = this.onclose;
            w.onmessage = this.onmessage;
            w.onerror = this.onerror;
            return w;
        },

        // websocket life message and timeout
        relaunchTimeout() {
            if (state.timeout) {
                clearTimeout(state.timeout);
            }
            state.timeout = setTimeout(() => {
                console.log("Websocket timeout");
                state.ws.close();  // Close socket to force reconnection
            }, 61 * 1000); // 61 seconds
        },

        // Bip
        bip() {
            state.audioBip = new Audio("bip.mp3");
            state.audioBip.play();
        },

        // Ring (server mode only)
        ring() {
            this.pauseAudioRing();
            this.ringing = true;
            state.audioRing = new Audio("ring.wav");
            state.audioRing.play();
        },

        // Stop ring
        stopring() {
            console.log("Stop ring...");
            this.bip();
            if (!this.mbModeClient) {
                this.pauseAudioRing();
            }
            else {
                this.get('/stopring');
            }
        },

        // Pause audio ring
        pauseAudioRing() {
            this.ringing = false;
            try {
                if (state.audioRing) state.audioRing.pause();
            }
            catch {
                console.log("Exception state.audioRing.pause()");
            }
        },

        // Listen
        listen(bip) {
            console.log("Listen...");
            if (bip) {
                this.bip();
            }
            this.get('/startlisten');
            this.listening = true;
            if (!this.mbModeClient) {
                this.pauseAudioRing();
                if (!state.audioContext) {
                    state.audioContext = new AudioContext();
                    console.log("state.audioContext:", state.audioContext);
                }
                state.audioProcess = state.audioContext.createScriptProcessor(state.frameBySample, 1, 1);
                state.audioProcess.connect(state.audioContext.destination);
                state.audioProcess.onaudioprocess = (e) => {
                    for (let i = 0; i < e.outputBuffer.length && state.recvQueue.length > 0; ++i) {
                        e.outputBuffer.getChannelData(0)[i] = state.recvQueue.shift();
                    }
                    if (this.recording) {
                        this.sendBuffer(e.inputBuffer);
                    }
                };
            }
        },

        // Speak
        speak(bip) {
            console.log("Speak...");
            if (bip) {
                this.bip();
            }
            this.get('/startspeaking');
            this.recording = true;
            this.listen(false);
            if (!this.mbModeClient) {
                this.pauseAudioRing();
                navigator.mediaDevices.getUserMedia({ audio: { channelCount: 1, echoCancellation: true, deviceId: this.deviceId } }).then((stream) => {
                    state.source = state.audioContext.createMediaStreamSource(stream);
                    state.source.connect(state.audioProcess);
                }).catch((err) => {
                    console.log("getUserMedia error: ", err);
                });
            }
        },

        // Convert buffer from float to int and send it on websocket
        sendBuffer(inputBuffer) {
            if (state.audioContext.sampleRate != state.rate) {
                // Convert rate
                const offlineCtx = new OfflineAudioContext(1, state.frameBySample / (state.audioContext.sampleRate / state.rate), state.rate);
                const source = offlineCtx.createBufferSource();
                source.buffer = inputBuffer;
                source.connect(offlineCtx.destination);
                source.start();
                offlineCtx.startRendering().then((renderedBuffer) => {
                    const buffer = renderedBuffer.getChannelData(0);
                    for (let i = 0; i < buffer.length; ++i) {
                        state.sendQueue.push(buffer[i] * 0x7FFF);
                        if (state.sendQueue.length == state.frameBySample) {
                            let sample = Int16Array.from(state.sendQueue);
                            state.ws.send(sample);
                            state.sendQueue = []; // Empty state.sendQueue
                        }
                    }
                }).catch((err) => {
                    console.log('Rendering failed: ' + err);
                });
            }
            else {
                const buffer = inputBuffer.getChannelData(0);
                let sampleArray = new Int16Array(buffer.length);
                for (let i = 0; i < buffer.length; ++i) {
                    sampleArray[i] = buffer[i] * 0x7FFF;
                }
                state.ws.send(sampleArray);
            }
        },

        // Open door
        dooropen(bip) {
            if (bip) {
                this.bip();
            }
            this.pauseAudioRing();
            this.get('/dooropen');
        },

        // Hangup
        hangup(bip) {
            if (bip) {
                this.bip();
            }
            this.pauseAudioRing();
            console.log("Hangup...");
            this.get('/hangup');

            // Stop audio process
            if (this.listening && !this.mbModeClient) {
                state.audioProcess.disconnect(state.audioContext.destination);
            }
            this.listening = false;
            if (this.recording && !this.mbModeClient) {
                state.source.disconnect(state.audioProcess);
            }
            this.recording = false;
            state.recvQueue = [];
            state.sendQueue = [];
        },

        // Save deviceID in localStorage
        saveDeviceId() {
            localStorage.setItem("deviceId", this.deviceId);
            console.log("this.deviceId:", this.deviceId);
        },

        // Send password to server
        async sendPassword(bip) {
            if (bip) {
                this.bip();
            }
            await fetch("/password", { method: "POST", body: this.password });
            this.checkPassword();
        },

        // Save password to local storage for next time
        savePassword(bip) {
            if (bip) {
                this.bip();
            }
            localStorage.setItem("password", this.password);
            this.bPasswordSaved = true;
        },

        // Clear password from local storage
        async disconnect(bip) {
            if (bip) {
                this.bip();
            }
            localStorage.removeItem("password");
            this.bPasswordSaved = false;
            await fetch("/password", { method: "DELETE" });
            this.checkPassword();
        },

        // Check if password is OK
        checkPassword() {
            console.log("checkPassword...");
            fetch("/password_ok").then((response) => response.text()).then(
                (text) => {
                    this.bPasswordOk = ("true" == text);
                }
            ).catch(() => {
                this.bPasswordOk = false;
            });
        },

        // Check if audio canal is busy
        checkAudioBusy() {
            console.log("checkAudioBusy...");
            fetch("/audiobusy").then((response) => response.text()).then(
                (text) => {
                    this.bAudioBusy = ("true" == text);
                }
            ).catch(() => {
                this.bAudioBusy = false;
            });
        },

        // Reload video when user click on image
        reloadVideo() {
            const videoSrcCopy = this.videoSrc;
            this.videoSrc = 'favicon.ico';
            state.videoReloadTimeout = setTimeout(() => {
                this.videoSrc = videoSrcCopy;
            }, 50);
        },
    },

    mounted() {
        // At startup, check if audio is busy
        this.checkAudioBusy();
    },
});

// Prevent drag and drop which can be annoying on touch screen.
// Turn screen backlight on.
app.directive("no-drag-drop", {
    mounted(el, binding) {
        const vm = binding.instance;
        el.addEventListener("dragstart", (event) => {
            console.log("dragstart");
            event.preventDefault();
            event.stopPropagation();
            vm.get("/backlighton");
        });
        el.addEventListener("click", () => {
            vm.get("/backlighton");
        });
    },
});

app.mount("#app");
