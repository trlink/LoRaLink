//globals
/////////
var g_strServer = "/";
var g_nLoadPos = 0;


$(document).ready(function() {    
    
    console.log("doc ready()");
    
    $("#dlgWait").show();
    $("#lblDesc").text("Load Web-Application data...");


    $("#dlgWait").hide();
    $("#loginPanel").show();

    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "GetDeviceCfg" ' +
              '}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            console.log(JSON.stringify(msg));

            if(msg["response"] == "OK") {

                $("#NodeName").text("Device Name: " + msg["szDevName"]);
                $("#NodeOwner").text("Node Owner: " + decodeURI(msg["szDevOwner"]));
            };
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));
        }
    });
});




function btnLogin_Click() {
    if($("#txtUsername").val().length > 0) {
        
        $("#dlgWait").show();
        $("#lblDesc").text("Validate User & Password...");
        

        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"command": "UserLogin", ' +
                  ' "Username": "' + $("#txtUsername").val() + '", ' +
                  ' "Password": "' + CryptoJS.MD5($("#txtPassword").val()) + '"}',
            contentType: 'application/json; charset=utf-8',
            crossDomain: true,
            dataType: 'json',
            async: true,
            headers: {
                "accept": "application/json",
                "Access-Control-Allow-Origin": "*",
                "Access-Control-Allow-Headers": "Content-Type, Accept, x-requested-with, x-requested-by",
                "Access-Control-Allow-Methods": "GET, POST"
            },
            success: function(msg) {
                console.log(JSON.stringify(msg));

                if(msg["response"] == "OK") {
                    
                    $("#loginPanel").hide();
                    
                    window.location.href = "./webMessenger.html?User=" + $("#txtUsername").val() + "&Pwd=" + CryptoJS.MD5($("#txtPassword").val()) + "&UserID=" + msg["UserID"];
                }
                else {
                    alert("You can't be logged in...");
                    $("#dlgWait").hide();
                };
            },
            error: function (msg) {
                
                alert("You can't be logged in, due to an error...");
                console.log(JSON.stringify(msg));

                $("#dlgWait").hide();
            }
        });
    };
};

