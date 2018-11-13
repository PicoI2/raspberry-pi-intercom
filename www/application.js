angular.module("ngApp", [])
.controller("intercomController", function($http, $timeout) {
    let me = this;
    me.stack = [];
    me.audioContext = new AudioContext();

    // Read mode
    me.mbModeClient = false;
    $http.get("mode").then(
        function (response) {
            if (response.data) {
                me.mbModeClient = ("client" == response.data);
                if (!me.mbModeClient) {
                    // Start weboscket
                    me.ws = connect();
                }
            }
        },
        function (response) {
            console.log("get mode (KO)", response);
        }
    );

    // Read framebysample
    me.frameBySample = 2048;
    $http.get("framebysample").then(
        function (response) {
            if (response.data) {
                me.frameBySample = parseInt(response.data);
                console.log("me.frameBySample:", me.frameBySample);
            }
        },
        function (response) {
            console.log("get framebysample (KO)", response);
        }
    );

    // Video src (motion) (IF SERVER)
    me.videoSrc = `http://${window.location.host.substr(0, window.location.host.lastIndexOf(':'))}:8081`;
    $http.get("videosrc").then(
        function (response) {
            if (response.data) {
                // Video src (motion) (IF CLIENT)
                me.videoSrc = `http://${response.data}:8081`;
            }
        },
        function (response) {
            console.log("get videosrc (KO)", response);
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
        me.wsConnected = true;
    };
    const onclose = function () {
        console.log('websocket onclose');
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
                let sampleIntArray = new Int16Array(fileReader.result);
                let sampleFloatArray = new Float32Array(sampleIntArray.length);
                for (let i=0; i<sampleIntArray.length; ++i) {
                    sampleFloatArray[i] = sampleIntArray[i] / 0x7FFF;
                }
                me.stack.push(sampleFloatArray);
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
            me.listenProcess = me.audioContext.createScriptProcessor(me.frameBySample, 0, 1);
            me.listenProcess.connect(me.audioContext.destination);
            me.listenProcess.onaudioprocess = function(e) {
                console.log("play...");
                let sampleFloatArray = me.stack.pop();
                if (sampleFloatArray) {
                    // console.log("pop OK");
                    for (let i=0; i<sampleFloatArray.length; ++i) {
                        e.outputBuffer.getChannelData(0)[i] = sampleFloatArray[i];
                    }
                }
                else {
                    console.log("nothing to read");
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

            navigator.mediaDevices.getUserMedia({ audio: true }).then( function(stream) {
                console.log("stream:", stream);
                
                me.source = me.audioContext.createMediaStreamSource(stream);
                me.speakProcess = me.audioContext.createScriptProcessor(me.frameBySample, 1, 0);
                me.source.connect(me.speakProcess);
                // me.speakProcess.connect(me.audioContext.destination);
                me.speakProcess.onaudioprocess = function(e) {
                    console.log("record...");
                    let sampleArray = new Int16Array(e.inputBuffer.getChannelData(0).length);
                    for (let i=0; i<e.inputBuffer.getChannelData(0).length; ++i) {
                        sampleArray[i] = e.inputBuffer.getChannelData(0)[i] * 0x7FFF;
                    }
                    me.ws.send(sampleArray);
                };
            }).catch(function(err) {
                console.log("getUserMedia error: ", err);
            });
        }
    };

    // Open door
    me.dooropen = function (bip) {
        if (bip) {
            me.bip();
        }
        if (me.audioRing) me.audioRing.pause();
        me.get('/dooropen');
    }

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
        if (me.listenProcess) me.listenProcess.disconnect(me.audioContext.destination);
        me.listening = false;
        me.stack = [];
    }
});