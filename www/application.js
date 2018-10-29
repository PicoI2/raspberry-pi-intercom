angular.module("ngApp", [])
.controller("intercomController", function($http) {
    let me = this;
    me.stack = [];

    // Video src (motion) (IF SERVER)
    me.videoSrc = `${window.location.protocol}//${window.location.host.substr(0, window.location.host.lastIndexOf(':'))}:8081`;
    $http.post("videosrc").then(
        function (response) {
            if (response.data) {
                // Video src (motion) (IF CLIENT)
                me.videoSrc = `${window.location.protocol}//${response.data}:8081`;
            }
        },
        function (response) {
            console.log("get videosrc (KO)", response);
        }
    );

    // Function to put msg on click
    me.put = function (msg) {
        $http.put(msg).then(
            function (response){
                console.log(`put ${msg} OK`);
            }, 
            function (response){
                console.log(`put ${msg} failed`);
            }
        );
    };

    // Websocket
    const onopen = function () {
        console.log('websocket onopen');
        // me.ws.send("hello");
    };
    const onclose = function () {
        console.log('websocket onclose');
        me.stop();
    };
    const onmessage = function (msg) {
        console.log('websocket onmessage');
        console.log(msg.data);
        let sampleIntArray = new Int16Array(msg.data);
        let sampleFloatArray = new Float32Array(sampleIntArray.length);
        for (let i=0; i<sampleIntArray.length; ++i) {
            sampleFloatArray[i] = sampleIntArray[i] / 0x7FFF;
        }
        console.log("sampleFloatArray", sampleFloatArray);
        me.stack.push(sampleFloatArray);
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
    me.ws = connect();
    
    // Call
    me.call = function () {
        console.log("Call...");
        navigator.mediaDevices.getUserMedia({ audio: true }).then( function(stream) {
            console.log("stream:", stream);
            me.audioContext = new AudioContext();
            me.source = me.audioContext.createMediaStreamSource(stream);
            me.processor = me.audioContext.createScriptProcessor(1024, 1, 1);
            me.source.connect(me.processor);
            me.processor.connect(me.audioContext.destination);
            me.processor.onaudioprocess = function(e) {
                let sampleArray = new Int16Array(e.inputBuffer.getChannelData(0).length);
                for (let i=0; i<e.inputBuffer.getChannelData(0).length; ++i) {
                    sampleArray[i] = e.inputBuffer.getChannelData(0)[i] * 0x7FFF;
                }
                me.ws.send(sampleArray);
                let sampleFloatArray = me.stack.pop();
                if (sampleFloatArray) {
                    for (let i=0; i<sampleFloatArray.length; ++i) {
                        e.outputBuffer.getChannelData(0)[i] = sampleFloatArray[i];
                    }
                }
            };
        }).catch(function(err) {
            console.log("getUserMedia error: ", err);
        });
    };

    me.stop = function () {
        me.source.disconnect(me.processor);
        me.processor.disconnect(me.audioContext.destination);
    }
});