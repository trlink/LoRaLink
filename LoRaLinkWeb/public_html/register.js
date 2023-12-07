//globals
/////////
var g_strServer = "/";
var g_nLoadPos = 0;


$(document).ready(function() {    
    
    console.log("doc ready()");

    $("#dlgWait").show();
    $("#lblDesc").text("Load Web-Application data...");
    $("#regPanel").hide();
    $("#dlgWait").hide();
    $("#regPanel").show();
});




function isUserNameValid(username) {
  /* 
    Usernames can only have: 
    - Lowercase Letters (a-z) 
    - Numbers (0-9)
    - Dots (.)
    - Underscores (_)
  */
  const res = /^[a-zA-Z0-9_\.\-]+$/.exec(username);
  const valid = !!res;
  return valid;
};


function btnRegister_Click() {
    //variables
    ///////////
    var strUser = $("#txtUsername").val();
    var strMail = encodeURI($("#txtMail").val());
    
    if((strUser.length > 0) && (strUser.length < 25) && (strMail.length < 100)) {
        if(isUserNameValid(strUser) === true) {
            if(($("#txtPassword").val() === $("#txtPasswordConf").val()) && ($("#txtPassword").val().length > 0)) {
                
                $("#dlgWait").show();
                $("#lblDesc").text("Validate registration Data...");
                $("#btnRegister").attr("disabled", true); 
    
                $.ajax({
                    url: g_strServer + 'api/api.json',
                    type: 'POST',
                    data: '{"command": "CreateUser", ' +
                          ' "UserName": "' + strUser + '", ' +
                          ' "mail": "' + strMail + '", ' +
                          ' "pwd": "' + CryptoJS.MD5($("#txtPassword").val()) + '"}',
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
                            $("#dlgWait").hide();

                            alert("Sucessfully registerred! - Please login on the main page!");
                        }
                        else {
                            $("#dlgWait").hide();
                            
                            $("#btnRegister").attr("disabled", false); 
                        };
                    },
                    error: function (msg) {
                        console.log(JSON.stringify(msg));

                        $("#dlgWait").hide();
                        $("#btnRegister").attr("disabled", false); 
                    }
                });
            }
            else {
                alert("Passwords doesn't match, or empty...");
            };
        }
        else {
            alert("Username only allows alphanumeric characters, numbers, dot & underscore...");
        };
    }
    else {
        alert("Username can't be empty, or is to long, or email is to long...");
    };
};