angular.module("ngApp", [])
.controller("intercomController", function ($http, $timeout) {
    let me = this;
    me.recvQueue = [];
    me.sendQueue = new Int16Array();
    me.videoSrc = `http://${window.location.host.substr(0, window.location.host.lastIndexOf(':'))}:8081`;

    me.recAudioContext = new AudioContext();
    console.log("me.recAudioContext:", me.recAudioContext);

    // Read configuration and then start websocket if needed
    $http.get("/config").then(
        function (response) {
            const config = response.data;
            console.log(config);
            if (response.data) {
                me.mbModeClient = ("client" == config.mode);
                me.frameBySample = config.frameBySample;
                me.rate = config.rate;
                me.videoSrc = `http://${config.videoSrc}:8081`;

                // Create audio context with sample rate
                me.playAudioContext = new AudioContext({
                    sampleRate: me.rate,
                });
                console.log("me.playAudioContext:", me.playAudioContext);

                if (!me.mbModeClient) {
                    // Start weboscket
                    me.ws = connect();
                }
            }
        },
        function (response) {
            me.message = "Failed to read configuration";
        }
    );

    // Simple http get function without waiting for result
    me.get = function (msg) {
        $http.get(msg).then(
            function (response){
                console.log(`get ${msg} OK`);
            }, 
            function (response){
                console.log(`get ${msg} failed`);
            }
        );
    };

    // Websocket
    const onopen = function () {
        console.log('websocket onopen');
        me.message = "Connected to server";
        me.wsConnected = true;
    };
    const onclose = function () {
        console.log('websocket onclose');
        me.message = "Disconnected from server";
        me.hangup(false);
        // Try to reconnect in 5 seconds
        $timeout(function () {
            me.ws = connect();
        }, 5000);
    };
    const onmessage = function (msg) {
        // console.log(typeof msg.data);
        // console.log(msg.data);
        if ("string" == typeof msg.data) {
            console.log(msg.data);
            if ("doorbell" == msg.data) {
                me.ring();
            }
            else if ("alive" == msg.data) {
                // me.ws.send("alive"); TODO ADD life message from client to server
                me.relaunchTimeout();
            }
        }
        else if (me.listening) {
            const fileReader = new FileReader();
            // onloadend will be called once readAsArrayBuffer has finished
            fileReader.onloadend = function () {
                // Convert from int to float
                let sampleIntArray = new Int16Array(fileReader.result);
                let sampleFloatArray = new Float32Array(sampleIntArray.length);
                for (let i=0; i<sampleIntArray.length; ++i) {
                    sampleFloatArray[i] = sampleIntArray[i] / 0x7FFF;
                }
                // If rate of audio context is not the same of sample, convert it to expected sample rate
                if (me.playAudioContext.sampleRate != me.rate) {
                    const offlineCtx = new OfflineAudioContext (1, me.frameBySample * (me.playAudioContext.sampleRate / me.rate) , me.playAudioContext.sampleRate);
                    const source = offlineCtx.createBufferSource();
                    source.buffer = me.playAudioContext.createBuffer(1, me.frameBySample, me.rate);
                    for (let i=0; i<sampleFloatArray.length; ++i) {
                        source.buffer.getChannelData(0)[i] = sampleFloatArray[i];
                    }
                    source.connect(offlineCtx.destination);
                    source.start();
                    offlineCtx.startRendering().then(function(renderedBuffer) {
                        for (let i=0; i<renderedBuffer.length; ++i) {
                            me.recvQueue.push(renderedBuffer.getChannelData(0)[i]);
                        }
                    }).catch(function(err) {
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
    const onerror = function () {
        console.log('websocket onerror');
    };
    const connect = function () {
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
    me.relaunchTimeout = function () {
        if (me.timeout) {
            $timeout.cancel(me.timeout);
        }
        me.timeout = $timeout(function () {
            console.log("Websocket timeout");
            me.wsConnected = false;
            me.ws.close();  // Close socket to force reconnection
        }, 61 * 1000); // 61 seconds
    }

    // Bip
    me.bip = function () {
        if (me.audioBip) me.audioBip.pause();
        me.audioBip = new Audio("bip.mp3");
        me.audioBip.play();
    }

    // Ring (server mode only)
    me.ring = function () {
        if (me.audioRing) me.audioRing.pause();
        me.audioRing = new Audio("ring.wav");
        me.audioRing.play();
    };

    // Stop ring
    me.stopring = function () {
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
    me.listen = function (bip) {
        console.log("Listen...");
        if (bip) {
            me.bip();
        }
        me.get('/startlisten');
        if (!me.mbModeClient) {
            if (me.audioRing) me.audioRing.pause();
            me.listening = true;
            me.listenProcess = me.playAudioContext.createScriptProcessor(me.frameBySample, 0, 1);
            me.listenProcess.connect(me.playAudioContext.destination);
            me.listenProcess.onaudioprocess = function(e) {
                for (let i=0; i<e.outputBuffer.length && me.recvQueue.length > 0; ++i) {
                    e.outputBuffer.getChannelData(0)[i] = me.recvQueue.shift();
                }
            };
        }
    };

    // Speak
    me.speak = function (bip) {
        console.log("Speak...");
        if (bip) {
            me.bip();
        }
        me.get('/startspeaking');
        me.listen(false);
        if (!me.mbModeClient) {
            if (me.audioRing) me.audioRing.pause();
            navigator.mediaDevices.getUserMedia({ audio: {sampleRate: me.rate, channelCount: 1, echoCancellation: true} }).then( function(stream) {
                me.source = me.recAudioContext.createMediaStreamSource(stream);
                me.speakProcess = me.recAudioContext.createScriptProcessor(me.frameBySample, 1, 0);
                me.source.connect(me.speakProcess);
                me.speakProcess.onaudioprocess = function(e) {
                    // console.log("record...");
                    me.sendBuffer(e.inputBuffer);
                };
            }).catch(function(err) {
                console.log("getUserMedia error: ", err);
            });
        }
    };

    // Convert buffer from float to int and send it on websocket
    me.sendBuffer = function (inputBuffer) {
        if (me.recAudioContext.sampleRate != me.rate) {
            // Convert rate TODO
            const offlineCtx = new OfflineAudioContext (1, me.frameBySample, me.rate);
            const source = offlineCtx.createBufferSource();
            // console.log("e.inputBuffer: ", e.inputBuffer);
            source.buffer = inputBuffer;
            source.connect(offlineCtx.destination);
            console.log('Start rendering...');
            source.start();
            offlineCtx.startRendering().then(function(renderedBuffer) {
                console.log('Rendering completed successfully');
                buffer = renderedBuffer.getChannelData(0)
                for (let i=0; i<buffer.length; ++i) {
                    me.sendQueue.push(buffer[i] * 0x7FFF);
                    if (me.sendQueue.length == me.frameBySample) {
                        me.ws.send(me.sendQueue);
                        me.sendQueue.splice(0, me.sendQueue.length) // Empty me.sendQueue
                    }
                }
            }).catch(function(err) {
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
    me.dooropen = function (bip) {
        if (bip) {
            me.bip();
        }
        if (me.audioRing) me.audioRing.pause();
        me.get('/dooropen');
    };

    // Hangup
    me.hangup = function (bip) {
        if (bip) {
            me.bip();
        }
        if (me.audioRing) me.audioRing.pause();
        console.log("Hangup...");
        me.get('/hangup');

        // Stop audio process
        if (me.source) me.source.disconnect(me.speakProcess);
        if (me.listenProcess) me.listenProcess.disconnect(me.playAudioContext.destination);
        me.listening = false;
        me.recvQueue = [];
    };
});
