angular.module("ngApp", [])
.controller("intercomController", function($http) {
    let me = this;
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
    };
    const onmessage = function (msg) {
        console.log('websocket onmessage');
        console.log(msg.data);
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
        navigator.mediaDevices.getUserMedia({ audio: true, video: false }).then( function(stream) {
            console.log("stream:", stream);
            var context = new AudioContext();
            var source = context.createMediaStreamSource(stream);
            var processor = context.createScriptProcessor(1024, 1, 1);
            source.connect(processor);
            processor.connect(context.destination);
            processor.onaudioprocess = function(e) {
                me.ws.send(e.inputBuffer.getChannelData(0));
            };
        }).catch(function(err) {
            /* handle the error */
        });
    };
});