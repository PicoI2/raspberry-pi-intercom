const ngApp = angular.module("ngApp", []);

ngApp.controller("intercomController", function ($http, $timeout, $scope) {
    let me = this;
    me.recvQueue = [];
    me.sendQueue = [];
    me.message = {
        color: "white",
        message: "starting",
    };

    // Useful in server mode, generate video URL
    const colonIndex = window.location.host.lastIndexOf(':');
    if (colonIndex > 0) {
        me.videoSrc = `http://${window.location.host.substr(0, colonIndex)}:8081`;
    }
    else {
        me.videoSrc = `http://${window.location.host}:8081`;
    }

    // Read configuration and then start websocket if needed
    $http.get("/config").then(
        (response) => {
            const config = response.data;
            console.log(config);
            if (response.data) {
                me.mbModeClient = ("client" == config.mode);
                me.frameBySample = config.frameBySample;
                me.rate = config.rate;
                if (me.mbModeClient) {
                    me.videoSrc = `http://${config.videoSrc}:8081`;
                }
                if (!me.mbModeClient) {
                    // Start weboscket
                    me.ws = connect();
                }
            }
        },
        (response) => {
            me.message.text = "Failed to read configuration";
            me.message.color = "red";
        }
    );

    // Simple http get function without waiting for result
    me.get = (msg) => {
        $http.get(msg).then(
            (response) => {
                console.log(`get ${msg} OK`);
            }, 
            (response) => {
                console.log(`get ${msg} failed`);
            }
        );
    };

    // Websocket
    function onopen () {
        console.log('websocket onopen');
        me.message.text = "Connected to server";
        me.message.color = "green";
        me.wsConnected = true;
        $scope.$apply();
    };
    function onclose () {
        console.log('websocket onclose');
        me.message.text = "Disconnected from server";
        me.message.color = "red";
        me.hangup(false);
        $scope.$apply();
        // Try to reconnect in 5 seconds
        $timeout(() => {
            me.ws = connect();
        }, 5000);
    };
    function onmessage (msg) {
        // console.log(typeof msg.data);
        // console.log(msg.data);
        if ("string" == typeof msg.data) {
            // console.log(msg.data);
            if ("doorbell" == msg.data) {
                me.ring();
            }
            else if ("audiobusy" == msg.data || "audiofree" == msg.data) {
                me.checkAudioBusy();
                $scope.$apply();
            }
            else if ("alive" == msg.data) {
                // me.ws.send("alive"); TODO ADD life message from client to server
                me.relaunchTimeout();
            }
        }
        else if (me.listening) {
            const fileReader = new FileReader();
            // onloadend will be called once readAsArrayBuffer has finished
            fileReader.onloadend = () => {
                // Convert from int to float
                let sampleIntArray = new Int16Array(fileReader.result);
                let sampleFloatArray = new Float32Array(sampleIntArray.length);
                for (let i=0; i<sampleIntArray.length; ++i) {
                    sampleFloatArray[i] = sampleIntArray[i] / 0x7FFF;
                }
                // If rate of audio context is not the same of sample, convert it to expected sample rate
                if (me.audioContext.sampleRate != me.rate) {
                    const offlineCtx = new OfflineAudioContext (1, me.frameBySample * (me.audioContext.sampleRate / me.rate) , me.audioContext.sampleRate);
                    const source = offlineCtx.createBufferSource();
                    source.buffer = me.audioContext.createBuffer(1, me.frameBySample, me.rate);
                    for (let i=0; i<sampleFloatArray.length; ++i) {
                        source.buffer.getChannelData(0)[i] = sampleFloatArray[i];
                    }
                    source.connect(offlineCtx.destination);
                    source.start();
                    offlineCtx.startRendering().then((renderedBuffer) => {
                        for (let i=0; i<renderedBuffer.length; ++i) {
                            me.recvQueue.push(renderedBuffer.getChannelData(0)[i]);
                        }
                    }).catch((err) => {
                        console.log('Rendering failed: ' + err);
                    });
                }
                else {
                    // No need to convert sample rate
                    for (let i=0; i<sampleFloatArray.length; ++i) {
                        me.recvQueue.push(sampleFloatArray[i]);
                    }   
                }
            }
            fileReader.readAsArrayBuffer(msg.data);
        }
    };
    function onerror () {
        console.log('websocket onerror');
    };
    function connect () {
        const wsurl = window.location.origin.replace("http", "ws");
        console.log('connect to ', wsurl);
        const w = new WebSocket(wsurl);
        w.onopen = onopen;
        w.onclose = onclose;
        w.onmessage = onmessage;
        w.onerror = onerror;
        return w;
    };

    // websocket life message and timeout
    me.relaunchTimeout = () => {
        if (me.timeout) {
            $timeout.cancel(me.timeout);
        }
        me.timeout = $timeout(() => {
            console.log("Websocket timeout");
            me.wsConnected = false;
            me.ws.close();  // Close socket to force reconnection
        }, 61 * 1000); // 61 seconds
    }

    // Bip
    me.bip = () => {
        if (me.audioBip) me.audioBip.pause();
        me.audioBip = new Audio("bip.mp3");
        me.audioBip.play();
    }

    // Ring (server mode only)
    me.ring = () => {
        if (me.audioRing) me.audioRing.pause();
        me.audioRing = new Audio("ring.wav");
        me.audioRing.play();
    };

    // Stop ring
    me.stopring = () => {
        console.log("Stop ring...");
        me.bip();
        if (!me.mbModeClient) {
            if (me.audioRing) me.audioRing.pause();
        }
        else {
            me.get('/stopring');
        }
    };
    
    // Listen
    me.listen = (bip) => {
        console.log("Listen...");
        if (bip) {
            me.bip();
        }
        me.get('/startlisten');
        me.listening = true;
        if (!me.mbModeClient) {
            if (me.audioRing) me.audioRing.pause();
            if (!me.audioContext) {
                me.audioContext = new AudioContext();
                console.log("me.audioContext:", me.audioContext);
            }
            me.audioProcess = me.audioContext.createScriptProcessor(me.frameBySample, 1, 1);
            me.audioProcess.connect(me.audioContext.destination);
            me.audioProcess.onaudioprocess = (e) => {
                // console.log("play...:");
                for (let i=0; i<e.outputBuffer.length && me.recvQueue.length > 0; ++i) {
                    e.outputBuffer.getChannelData(0)[i] = me.recvQueue.shift();
                }
                if (me.recording) {
                    // console.log("record...");
                    me.sendBuffer(e.inputBuffer);
                }
            };
        }
    };

    // Speak
    me.speak = (bip) => {
        console.log("Speak...");
        if (bip) {
            me.bip();
        }
        me.get('/startspeaking');
        me.recording = true;
        me.listen(false);
        if (!me.mbModeClient) {
            if (me.audioRing) me.audioRing.pause();
            navigator.mediaDevices.getUserMedia({ audio: {channelCount: 1, echoCancellation: true, deviceId: me.deviceId} }).then( function(stream) {
                me.source = me.audioContext.createMediaStreamSource(stream);
                me.source.connect(me.audioProcess);
            }).catch((err) => {
                console.log("getUserMedia error: ", err);
            });
        }
    };

    // Convert buffer from float to int and send it on websocket
    me.sendBuffer = (inputBuffer) => {
        if (me.audioContext.sampleRate != me.rate) {
            // Convert rate
            const offlineCtx = new OfflineAudioContext (1, me.frameBySample / (me.audioContext.sampleRate / me.rate), me.rate);
            const source = offlineCtx.createBufferSource();
            // console.log("e.inputBuffer: ", e.inputBuffer);
            source.buffer = inputBuffer;
            source.connect(offlineCtx.destination);
            // console.log('Start rendering...');
            source.start();
            offlineCtx.startRendering().then((renderedBuffer) => {
                // console.log('Rendering completed successfully');
                buffer = renderedBuffer.getChannelData(0)
                for (let i=0; i<buffer.length; ++i) {
                    me.sendQueue.push(buffer[i] * 0x7FFF);
                    if (me.sendQueue.length == me.frameBySample) {
                        let sample = Int16Array.from(me.sendQueue);
                        me.ws.send(sample);
                        me.sendQueue = []; // Empty me.sendQueue
                    }
                }
            }).catch((err) => {
                console.log('Rendering failed: ' + err);
            });
        }
        else {
            buffer = inputBuffer.getChannelData(0)
            let sampleArray = new Int16Array(buffer.length);
            for (let i=0; i<buffer.length; ++i) {
                sampleArray[i] = buffer[i] * 0x7FFF;
            }
            me.ws.send(sampleArray);
        }
    };

    // Open door
    me.dooropen = (bip) => {
        if (bip) {
            me.bip();
        }
        if (me.audioRing) me.audioRing.pause();
        me.get('/dooropen');
    };

    // Hangup
    me.hangup = (bip) => {
        if (bip) {
            me.bip();
        }
        if (me.audioRing) me.audioRing.pause();
        console.log("Hangup...");
        me.get('/hangup');

        // Stop audio process
        if (me.listening && !me.mbModeClient) {
            me.audioProcess.disconnect(me.audioContext.destination);
        }
        me.listening = false;
        if (me.recording && !me.mbModeClient) {
            me.source.disconnect(me.audioProcess);
        }
        me.recording = false;
        me.recvQueue = [];
        me.sendQueue = [];
    };

    // List availables "input devices". Useful for chrome but not for firefox
    me.devices = [];
    navigator.mediaDevices.enumerateDevices().then((devices) => {
        devices.forEach ((device) => {
            if (device.kind == "audioinput" && device.deviceId != "default") {
                me.devices.push(device);
            }
        });
        console.log("devices:", me.devices);
        // Read device ID in localStorage
        me.deviceId = localStorage.getItem("deviceId");
        console.log("me.deviceId:", me.deviceId);
        $scope.$apply()
    });

    // Save deivceID in localStorage
    me.saveDeviceId = () => {
        localStorage.setItem("deviceId", me.deviceId);
        console.log("me.deviceId:", me.deviceId);
    };

    // Send password to server
    me.sendPassword = async (bip) => {
        if (bip) {
            me.bip();
        }
        await $http.post("/password", me.password);
        me.checkPassword();
    }

    // Save password to local storage for next time
    me.savePassword = (bip) => {
        if (bip) {
            me.bip();
        }
        localStorage.setItem("password", me.password);
        me.bPasswordSaved = true;
    }

    // Clear password from local storage
    me.disconnect = async (bip) => {
        if (bip) {
            me.bip();
        }
        localStorage.removeItem("password");
        me.bPasswordSaved = false;
        await $http.delete("/password");
        me.checkPassword();
    }

    // Check if password is OK
    me.checkPassword = () => {
        console.log("checkPassword...");
        $http.get("/password_ok").then(
            (response) => {
                me.bPasswordOk = ("true" == response.data);
            },
            (response) => {
                me.bPasswordOk = false;
            }
        );
    }

    // At statup, load saved password
    me.password = localStorage.getItem("password");
    me.bPasswordSaved = true;
    me.sendPassword(false);

    // Check if audio canal is busy
    me.checkAudioBusy = () => {
        console.log("checkAudioBusy...");
        $http.get("/audiobusy").then(
            (response) => {
                me.bAudioBusy = ("true" == response.data);
            },
            (response) => {
                me.bAudioBusy = false;
            }
        );
    }

    // At startup, check if audio is busy
    me.checkAudioBusy();

    // Reload video when user click on image
    me.reloadVideo = () => {
        me.videoSrcCopy = me.videoSrc;
        me.videoSrc = 'favicon.ico';
        $timeout(() => {
            me.videoSrc = me.videoSrcCopy;
        }, 50);
    };
    
});

// Prefent drag and drop wich can be annoying on touch screen
// Turn screen backlight on
ngApp.directive('noDragDrop', function () {
    return {
        restrict: 'A',
        link: (scope, element, attrs) => {
            element.bind("dragstart", (event) => {
                console.log("dragstart");
                event.preventDefault();
                event.stopPropagation();
                scope.ctrl.get("/backlighton");
            });
            element.bind("click", (event) => {
                scope.ctrl.get("/backlighton");
            });
        }
    }; 
});
