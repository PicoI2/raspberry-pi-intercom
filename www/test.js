angular.module("ngApp", [])
.controller("testController", function ($http, $timeout, $scope) {
    let me = this;

    me.listDevices = function () {
        navigator.mediaDevices.enumerateDevices().then(function(devices) {
            me.devices = devices;
            $scope.$apply();
            devices.forEach(function(device) {
                console.log(device);
            });
        });
    };

    navigator.mediaDevices.getUserMedia({ audio: true}).then( function(stream) {
        me.listDevices();
    }).catch(function(err) {
        me.listDevices();
    });

    // Start
    me.start = function () {
        console.log("Start...");
        me.audioContext = new AudioContext();
        navigator.mediaDevices.getUserMedia({ audio: { deviceId: me.input }}).then( function(stream) {
            me.source = me.audioContext.createMediaStreamSource(stream);
            me.source.connect(me.audioContext.destination);
        }).catch(function(err) {
            console.log("getUserMedia error: ", err);
        });
    };

    // Stop
    me.stop = function () {
        // Stop audio process
        if (me.source) me.source.disconnect(me.audioContext.destination);
    };
});
