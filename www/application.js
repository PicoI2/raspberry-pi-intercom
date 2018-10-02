angular.module("ngApp", [])
.controller("intercomController", function($http) {
    let me = this;
    me.stopring = function () {
        $http.put("/stopring").then(
            function (response){
                console.log("post stopring OK");
            }, 
            function (response){
                console.log("post stopring failed");
            }
        );
    };
});