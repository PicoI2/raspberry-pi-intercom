angular.module("ngApp", [])
.controller("intercomController", function($http) {
    let me = this;
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
});