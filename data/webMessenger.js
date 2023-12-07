//globals
/////////
var g_nAppMinWidth    = 900;
var g_strServer       = "/";
var g_nLoadPos = 0;
var MAX_MESSAGE_CHARS = 1500;
var g_aKnownNodes     = [];
var g_aChats          = [];
var g_aContacts       = [];
var g_aChatMsgs       = [];
var g_bMessageView    = false;
var g_bShoutoutLoaded = false;
var g_dwLastShoutOut  = 0;
var g_bConnected      = false;
//gps stuff
var g_bHaveGPS        = false;
var g_bGpsValid       = false;
var g_fLocalAlt       = 0.0;
var g_fLocalLat       = 0.0;
var g_fLocalLon       = 0.0;
var g_fLocalCourse    = 0.0;
var g_fLocalSpeed     = 0.0;
var g_nLocalSat       = 0;
var g_nLocalHDOP      = 0;
var g_lLocalTimeStamp = 0;
var g_bTrackingActive = false;
var g_nTrackingType   = 0;
var g_pRadar          = null;


var strChatHeadEntry = `<tr class="divChatHeadContainerEntry" onclick="javascript: toggleMessageView(true); loadChatMsgs({CHATID}, true); $('#tbShoutOut').hide(); $('#tbChatMsgs').show();">
                            <td>
                                <table style="position: relative; width: 100%; table-layout: fixed;">
                                    <tr>
                                        <td>
                                            <span class="divChatHeadContainerEntryHeader {divChatHeadContainerUserName}">{USERNAME}</span>@<span class="divChatHeadContainerEntryHeader {divChatHeadContainerNode}">{NODENAME},</span>
                                            <span class="divChatHeadContainerEntryHeader divChatHeadContainerTime">{TIME}</span>
                                        </td>
                                    </tr>

                                    <tr>
                                        <td>
                                            <p class="divChatHeadContainerEntryHeader divChatHeadContainerLastMsg">{LASTMSG}</p>
                                        </td>
                                    </tr>

                                    <tr>
                                        <td class="text-right">
                                            <span class="badge badge-danger {MSGBADGECLASS} text-right" style="position: relative;">{NEWMSGS}</span>
                                        </td>
                                    </tr>
                                </table>
                            </td>
                        </tr>`;


var strChatMsgEntry =  `<tr class="rowChatMsgContainerEntry">
                            <td>
                                <table style="width: 100%; table-layout: fixed;">
                                    <tr class="rowChatMsgHeader rowChatMsgHeaderTop">

                                        <td class="{DIRCLASSLEFT}">
                                        </td>

                                        <td>
                                            <p class="lblChatMsgContainerUserName">{USERNAME}</p>
                                        </td>

                                        <td class="text-right">
                                            <p class="lblChatMsgContainerTime">{TIME}</p>
                                        </td>

                                        <td class="rowChatRead">
                                        </td>

                                        <td class="{DIRCLASSRIGHT}">
                                        </td>
                                    </tr>
                                    <tr class="rowChatMsgHeader" width="100%">
                                        <td  class="{DIRCLASSLEFT}">
                                        </td>

                                        <td colspan="2" class="{MSGCLASS}" style="word-wrap: break-word;">    
                                            <p class="lblChatMsgContainerMsg">{MSG}</p>
                                        </td>

                                        <td class="rowChatRead">
                                            <img class="{MSGSTATE}" src="{MSGSTATEIMG}"></img>
                                        </td>

                                        <td class="{DIRCLASSRIGHT}">
                                        </td>
                                    </tr>
                                </table>
                            </td>
                        </tr>
                        `;




$(document).ready(function() {    
    
    console.log("doc ready()");
    
    $("#dlgWait").show();
    $("#lblDesc").text("Loading scripts and data...");
    $("#imgDeleteMsgs").attr("src", "delete.png");
    $("#imgBlockContact").attr("src", "block-user.png");
    $("#imgUnBlockContact").attr("src", "unblock-user.png");
    $("#imgShoutOutBack").attr("src", "back.png");
    $("#imgMsgsBack").attr("src", "back.png");
    $("#imgAllowPosTracking").attr("src", "allowTracking.png");
    $("#imgDisallowPosTracking").attr("src", "disallowTracking.png");
    $("#imgGpsState").attr("src", "invalidGPS.png");
    $("#imgConnState").attr("src", "disconnected.png");
    $("#imgLocationTracking").attr("src", "locationTracking.png");
    $("#imgLocationTracking").hide();
    
    onLoad();
    
    g_pRadar = new CRadar(document.getElementById("radar"));
    g_pRadar.disableCourse(true);
});




function initWebEventReader() {
    
    console.log("Init event reader...");
    
    //init event handler
    var init = setInterval(function () {
        readWebEvents();
    }, 1000);
};



function readWebEvents() {
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"command": "readWebEvents", ' +
              ' "userID": ' + $("#hfUserID").val() + ', ' +
              ' "hash": "' + $("#hfPwdHash").val() + '"}',
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
            
            //check if device is connected
            if(msg["bConnected"] !== g_bConnected) {
                g_bConnected = msg["bConnected"];
                
                if(g_bConnected === true) {
                    $("#imgConnState").attr("src", "connected.png");
                }
                else {
                    $("#imgConnState").attr("src", "disconnected.png");
                };
            };
            
            //gps stuff:
            //does the device have a GPS RX connected
            //if not disable all GPS controls
            if(msg["bHaveGPS"] === false) {
                $("#imgAllowPosTracking").hide();
                $("#imgDisallowPosTracking").hide();
                $("#imgGpsState").hide();
                $("#imgLocationTracking").hide();
                $("#mnuItemGPS").hide();
                
                g_bHaveGPS = false;
                g_bGpsValid = false;
            }
            else {
                g_bHaveGPS = true;
                
                //does the connected dev has a valid GPS signal?
                if(msg["bValidSignal"] === true) {
                    $("#imgGpsState").attr("src", "validGPS.png");

                    //set course and enable direction 
                    //arrow
                    if((parseInt(msg["fCourse"]) <= 360) && (parseInt(msg["fCourse"]) >= 0)) {
                        g_pRadar.disableCourse(false);
                        g_pRadar.setCourse(Math.round(msg["fCourse"]));
                    }
                    else {
                        g_pRadar.disableCourse(true);
                    };
                }
                else {
                    g_pRadar.disableCourse(true);

                    $("#imgGpsState").attr("src", "invalidGPS.png");
                };

                g_bGpsValid = msg["bValidSignal"];

                
                if((msg["bTrackingActive"] !== g_bTrackingActive) || (g_nTrackingType !== msg["nTrackingType"])) {
                    if(msg["bTrackingActive"] === true) {
                        
                        $("#imgLocationTracking").show();
                        
                        if(msg["nTrackingType"] === 0) {
                            $("#imgLocationTracking").attr("src", "locationTracking.png");
                        }
                        else {
                            $("#imgLocationTracking").attr("src", "emergency-icon.png");
                        };

                        g_nTrackingType = msg["nTrackingType"];
                    }
                    else {
                        $("#imgLocationTracking").hide();
                    };
                    
                    g_bTrackingActive = msg["bTrackingActive"];
                };
            };
            
            switch(msg["nEventID"]) {
                
                //new chat message
                case 1: {
                    console.log("Message received...");
                    
                    loadChatsFromDevice(true);
                    
                    if($("#hfSelectedChatID").val().length > 0) {
                        if(parseInt($("#hfSelectedChatID").val()) > 0) {
                            console.log("reload chat msgs for: " + $("#hfSelectedChatID").val());

                            loadChatMsgs($("#hfSelectedChatID").val(), false);
                        };
                    };
                };
                break;
                
                //outgoing message stored on device
                case 2: {
                    loadUserContacts(true);
                    loadChatsFromDevice(true);
                    
                    if($("#hfSelectedChatID").val().length > 0) {
                        
                        if(parseInt($("#hfSelectedChatID").val()) > 0) {
                            console.log("reload chat msgs for: " + $("#hfSelectedChatID").val());

                            loadChatMsgs($("#hfSelectedChatID").val(), false);
                        };
                    };

                    $("#dlgWait").hide();
                    $("#dlgNewChat").modal("hide");
                    $("#btnSendMessage2").attr("disabled", false);
                };
                break;
                
                //unable to save msg
                case 3: {
                    alert("Unable to store message");
                    
                    $("#dlgWait").hide();
                    $("#dlgNewChat").modal("hide");  
                    $("#btnSendMessage2").attr("disabled", false);
                };
                break;
                
                //saved outgoing shout out
                case 6: {
                    $("#txtMsgTextShoutOut").val("");
                    
                    if(g_bShoutoutLoaded === true) {
                        
                        if(msg["dwDataID"] !== g_dwLastShoutOut) {
                            loadShoutOutMsgs(msg["dwDataID"]);   

                            g_dwLastShoutOut = msg["dwDataID"];
                        };
                    }
                    else {
                        loadShoutOutMsgs(0);
                    };
                };
                break;
                
                //incoming shout out
                case 5: {
                    if(g_bShoutoutLoaded === true) {
                        if(msg["dwDataID"] !== g_dwLastShoutOut) {
                            loadShoutOutMsgs(msg["dwDataID"]);   

                            g_dwLastShoutOut = msg["dwDataID"];
                        };   
                    }
                    else {
                        loadShoutOutMsgs(0);
                    };
                };
                break;
              
                //received GPS position
                case 7: {
                    if(msg["szData"] !== null) {
                        console.log("Received GPS Data: " + msg["szData"]);
                        
                        const gps = JSON.parse(msg["szData"]);
                        
                        console.log("Set sender " + gps.Sender + " course to " + gps.Course2 + " Distance: " + gps.Dst);
                        
                        g_pRadar.addPoint(gps);
                    };
                };
                break;
            };
        },
        error: function (msg) {

            console.log("Unable to read events");
            console.log(JSON.stringify(msg));
            
            g_bConnected = false;
                
            if(g_bConnected === true) {
                $("#imgConnState").attr("src", "connected.png");
            }
            else {
                $("#imgConnState").attr("src", "disconnected.png");
            };
        }
    });
};


function getRequests() {
    var s1 = location.search.substring(1, location.search.length).split('&'),
        r = {}, s2, i;
    for (i = 0; i < s1.length; i += 1) {
        s2 = s1[i].split('=');
        r[decodeURIComponent(s2[0]).toLowerCase()] = decodeURIComponent(s2[1]);
    }
    return r;
};


function resizeWindow() {
  
    $("#divMain").height($(window).height() - ($("#navMainMenu").height() + 17));
    $("#txtMsgTextShoutOut").width($("#tbShoutOut").width() - 133);
    
    toggleMessageView(g_bMessageView);
};



function toggleMessageView(bOpenMessageView) {
    console.log("App width:" + $(window).width());
    
    g_bMessageView = bOpenMessageView;
    
    if($(window).width() < g_nAppMinWidth) {
        
        $("#tbBackBtn").show();
        $("#tbBackBtn2").show();
        
        if(bOpenMessageView === true) {
            $("#divMainLeftPane").hide();
            $("#divMainRightPane").show();
            $("#divMainRightPane").width($(window).width());
        }
        else {
            $("#divMainLeftPane").show();
            $("#divMainLeftPane").width($(window).width());
            $("#divMainRightPane").hide();
        };
    }
    else {
        $("#tbBackBtn").hide();
        $("#tbBackBtn2").hide();
        $("#divMainLeftPane").show();
        $("#divMainRightPane").show();
        $("#divMainLeftPane").width(300);
        $("#divMainRightPane").width($(window).width() - $("#divMainLeftPane").width());
    };
};




function onLoad() {
    //variables
    ///////////
    var QueryString = getRequests();
    
    $("#hfPwdHash").val(QueryString["pwd"]);
    $("#hfUser").val(QueryString["user"]);
    $("#hfUserID").val(QueryString["userid"]);
    $("#hfSelectedChatID").val("0");
  
    if(($("#hfUserID").val().length <= 0) || ($("#hfUser").val().length <= 0) || ($("#hfPwdHash").val().length <= 0)) {
        window.location.href = "./index.html";
    }
    else {
        resizeWindow();
        
        loadUserContacts(false);
        
        initWebEventReader();
    };
};



$(window).on("resize", function() {
    resizeWindow();
});


function loadFinished() {
    console.log("load finished...");
    
    //hide after load
    $("#dlgWait").hide();
}



function showNewChatDialog() {
    $("#txtMsgTo").val("");
    $("#txtMsgText").val("");
    $("#dlgNewChat").modal('show');
    
    
    $('#divRcptResult').empty();
    $('#divRcptResult').hide();
    
    $("#btnSendMessage").attr("disabled", false);
}



function autocompleteMatch(input) {
    var res = [];
    
    if (input === '') {
        return [];
    };
    
    console.log("match for:" + input);

    var reg = new RegExp(input);
    
    for(i = 0; i < g_aContacts.length; ++i) {
        if ((g_aContacts[i].Name.match(reg)) || (g_aContacts[i].Device.match(reg))) {
            res.push(g_aContacts[i].Name + "@" + g_aContacts[i].Device);
        };
    };
    
 
    return res;
};


function showRcptResults(value) {
    let res = $('#divRcptResult');
    res.empty();
    
    let list = '';
    let terms = autocompleteMatch(value);
    
    for (i = 0; i < terms.length; i++) {
        console.log("add: " + terms[i]);
        list += '<li onClick="checkContact(\'' + terms[i] + '\'); $(\'#txtMsgTo\').val(\'' + terms[i] + '\'); $(\'#divRcptResult\').hide();">' + terms[i] + '</li>';
    }
    
    $('#divRcptResult').show();
    res.html('<ul>' + list + '</ul>');
}


function updateMessageLabel() {
    if(typeof $("#txtMsgTextHelp") !== "undefined") {
        $("#txtMsgTextHelp").text("Message text, 1500 chars max. (" + (MAX_MESSAGE_CHARS - encodeURI($("#txtMsgText").val()).length).toString() + " chars left...)");
    }
}




function loadUserContacts(bUpdateOnly) {
    
    $("#lblDesc").text("Loading user contacts...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"command": "loadUserContacts", ' +
              ' "userID": ' + $("#hfUserID").val() + ', ' +
              ' "hash": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        crossDomain: true,
        dataType: 'json',
        async: (bUpdateOnly === true ? false : true),
        headers: {
            "accept": "application/json",
            "Access-Control-Allow-Origin": "*",
            "Access-Control-Allow-Headers": "Content-Type, Accept, x-requested-with, x-requested-by",
            "Access-Control-Allow-Methods": "GET, POST"
        },
        success: function(msg) {
            console.log(JSON.stringify(msg));

            if(msg["response"] == "OK") {
                g_aContacts = msg["contacts"];
            };
            
            loadChatsFromDevice(bUpdateOnly);
        },
        error: function (msg) {

            console.log(JSON.stringify(msg));

            loadChatsFromDevice(false);
        }
    });
}


function chatHeadComparer( a, b ) {
  if ( Date.parse(a.StartTime) < Date.parse(b.StartTime )) {
    return -1;
  }
  if ( a.ChatRcpt > b.ChatRcpt ){
    return 1;
  }
  return 0;
}



function encryptString(input, key) {
    var c = '';
    while (key.length < input.length) {
         key += key;
    }
    
    for(var i=0; i<input.length; i++) {
        var value1 = input[i].charCodeAt(0);
        var value2 = key[i].charCodeAt(0);

        var xorValue = value1 ^ value2;

        var xorValueAsHexString = xorValue.toString("16");

        if (xorValueAsHexString.length < 2) {
            xorValueAsHexString = "0" + xorValueAsHexString;
        }

        c += xorValueAsHexString;
    };
    
    
    return c;
};



function decryptString(input, key) {
    var strKey = String(key);
    let c = '';
    
    while (strKey.length < input.length / 2) {
        strKey += strKey;
    }

    for (let i = 0; i < input.length; i += 2) {
        let hexValueString = input.substring(i, i + 2);
        let value1 = parseInt(hexValueString, 16);
        let value2 = strKey.charCodeAt(i / 2);

        let xorValue = value1 ^ (value2 & 0xFF);

        c += String.fromCharCode(xorValue);
    }
    
    return c;
};




function loadChatsFromDevice(bUpdateOnly) {
    
    $("#lblDesc").text("Loading chats and data...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"command": "loadChatHead", ' +
              ' "userID": ' + $("#hfUserID").val() + ', ' +
              ' "hash": "' + $("#hfPwdHash").val() + '"}',
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
            //variables
            ///////////
            var strTemp = "";
            var strHTML = "";
            var strUser = "";
            var strNode = "";
            var oContact = {};
            var strMsg   = "";
            
            console.log(JSON.stringify(msg));

            //clear table
            $('#tbChats tbody').empty();


            if(msg["response"] == "OK") {
                
                g_aChats = msg["chatHeads"];
                g_aChats.sort(chatHeadComparer);
                
                for(let n = 0; n < g_aChats.length; ++n) {
                    
                    oContact = getContact(g_aChats[n].ContactID);
                    
                    if(oContact !== null) {
                        strUser = oContact.Name;
                        strNode = oContact.Device;
                    }
                    else {
                        strUser = "Unknown";
                        strNode = "Unknown";
                    };
                    
                    strMsg  = decodeURI(g_aChats[n].LastMsgText);
                    
                    if(strMsg.startsWith("#enc:") === true) {
                        strMsg  = "*** Protected ***";
                    };
                    
                    strTemp = strChatHeadEntry;
                    strTemp = strTemp.replace("{USERNAME}", strUser);
                    strTemp = strTemp.replace("{NODENAME}", strNode);
                    strTemp = strTemp.replace("{TIME}", g_aChats[n].LastMsgTime);
                    strTemp = strTemp.replace("{CHATID}", g_aChats[n].ID);
                    strTemp = strTemp.replace("{LASTMSG}", strMsg);
                    strTemp = strTemp.replace("{NEWMSGS}", g_aChats[n].UnreadMsgs);
                    
                    if(oContact !== null) {
                        if(oContact.DeviceID > 0) {
                            strTemp = strTemp.replace("{divChatHeadContainerNode}", "divChatHeadContainerNodeNameKnown");
                        }
                        else {
                            if(oContact.State == 0) {
                                strTemp = strTemp.replace("{divChatHeadContainerNode}", "divChatHeadContainerNodeNameUnknown");
                            }
                            else {
                                strTemp = strTemp.replace("{divChatHeadContainerNode}", "divChatHeadContainerNodeNameError");
                            }
                        }

                        if(oContact.UserID > 0) {
                            strTemp = strTemp.replace("{divChatHeadContainerUserName}", "divChatHeadContainerNodeNameKnown");
                        }
                        else {
                            if(oContact.State == 0) {
                                strTemp = strTemp.replace("{divChatHeadContainerUserName}", "divChatHeadContainerNodeNameUnknown");
                            }
                            else {
                                strTemp = strTemp.replace("{divChatHeadContainerUserName}", "divChatHeadContainerNodeNameError");
                            }
                        }
                    }
                    else {
                        strTemp = strTemp.replace("{divChatHeadContainerNode}", "divChatHeadContainerNodeNameError");
                        strTemp = strTemp.replace("{divChatHeadContainerUserName}", "divChatHeadContainerNodeNameError");
                    };
                    
                    if(g_aChats[n].UnreadMsgs > 0) {
                        strTemp = strTemp.replace("{MSGBADGECLASS}", "");
                    }
                    else {
                        strTemp = strTemp.replace("{MSGBADGECLASS}", "msgBadgeInvisible");
                    }
                    
                    strHTML += strTemp;
                };
                
                $('#tbChats tbody').html(strHTML);
            };
            
            if(bUpdateOnly === false) {
                loadKnownDevices();
            };
        },
        error: function (msg) {

            alert("Unable to load messages...");
            console.log(JSON.stringify(msg));

            $("#dlgWait").hide();

            //window.location.replace("./index.html");
        }
    });
};


function loadKnownDevices() {
    
    $("#lblDesc").text("Load existing nodes...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"command": "getKnownNodes", ' +
              ' "userID": ' + $("#hfUserID").val() + ', ' +
              ' "hash": "' + $("#hfPwdHash").val() + '"}',
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

            g_aKnownNodes = msg["Nodes"];
            
            loadFinished();
        },
        error: function (msg) {

            console.log(JSON.stringify(msg));

            loadFinished();
        }
    });
}



function chatMsgComparer( a, b ) {
  return Date.parse(a.MsgTime) - Date.parse(b.MsgTime);
}


function shoutOutComparer( a, b ) {
  return Date.parse(a.SentTime) - Date.parse(b.SentTime);
}


function getContact(dwID) {
    for(let i = 0; i < g_aContacts.length; ++i) {
        if(parseInt(g_aContacts[i].ID) === parseInt(dwID)) {
            return g_aContacts[i];
        };
    };
    
    return null;
}


function getContactByString(strContact) {
    //variables
    ///////////
    var strTemp = "";
    
    for(let i = 0; i < g_aContacts.length; ++i) {
        strTemp = g_aContacts[i].Name + "@" + g_aContacts[i].Device;
        
        if(strTemp.toUpperCase() === strContact.toUpperCase()) {
            return g_aContacts[i];
        };
    };
    
    return null;
}


function deleteMsgs(chatID) {

    if(g_aChatMsgs.length > 0) {
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"command": "clearChat", ' +
                  ' "userID": ' + $("#hfUserID").val() + ', ' +
                  ' "chatID": ' + chatID + ', ' +
                  ' "hash": "' + $("#hfPwdHash").val() + '"}',
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
                $('#tbChatMsgsBody').empty();

                loadChatsFromDevice(true);
                toggleMessageView(false);
            },
            error: function (msg) {

                alert("Unable to delete messages...");
                console.log(JSON.stringify(msg));

                $("#hfSelectedChatID").val("0");
            }
        });
    }
    else {
        $("#hfSelectedChatID").val(0);
        $('#tbChatMsgs').hide();
        $('#tbChatMsgsBody').empty();
        
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"command": "deleteChat", ' +
                  ' "userID": ' + $("#hfUserID").val() + ', ' +
                  ' "chatID": ' + chatID + ', ' +
                  ' "hash": "' + $("#hfPwdHash").val() + '"}',
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
                loadChatsFromDevice(true);
                toggleMessageView(false);
            },
            error: function (msg) {

                alert("Unable to delete message...");
                console.log(JSON.stringify(msg));

                $("#hfSelectedChatID").val("0");
            }
        });
    };
    
};


function loadChatMsgs(chatID, bResetNew) {
    //variables
    ///////////
    var chat = getChatHeadByID(chatID);
    var oContact = null;
    
    
    g_aChatMsgs = [];
    
    //clear table
    $('#tbChatMsgs').show();
    $('#tbChatMsgsBody').empty();
    $("#hfSelectedChatID").val(chatID);
    
    if(chat !== null) {
        
        $("#lblChatName").text(chat.ChatRcpt);
        
        oContact = getContact(chat.ContactID);  
        
        if(oContact !== null) {
            if(oContact.Blocked === 1) {
                $("#txtMsgText2").attr("disabled", true);
                $("#btnSendMessage2").attr("disabled", true);
                $("#imgUnBlockContact").show();
                $("#imgBlockContact").hide();
            }
            else {
                $("#txtMsgText2").attr("disabled", false);
                $("#btnSendMessage2").attr("disabled", false);
                $("#imgUnBlockContact").hide();
                $("#imgBlockContact").show();
            };
            
            if(g_bHaveGPS === true) {
                if(oContact.AllowTracking === 1) {
                    $("#imgDisallowPosTracking").show();
                    $("#imgAllowPosTracking").hide();
                }
                else {
                    $("#imgDisallowPosTracking").hide();
                    $("#imgAllowPosTracking").show();
                };
            };
        }
        else {
            $("#txtMsgText2").attr("disabled", false);
            $("#btnSendMessage2").attr("disabled", false);
            $("#imgUnBlockContact").hide();
            $("#imgBlockContact").show();
            
            if(g_bHaveGPS === true) {
                $("#imgAllowPosTracking").show();
            };
        };

        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"command": "loadChatMsgs", ' +
                  ' "userID": ' + $("#hfUserID").val() + ', ' +
                  ' "chatID": ' + chatID + ', ' +
                  ' "hash": "' + $("#hfPwdHash").val() + '"}',
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
                //variables
                ///////////
                var strHTML = "";
                var strTemp = "";
                var contact = {};
                var bEncrypt = $('#chkEncryptMessage').prop('checked');
                var strKey   = String(CryptoJS.MD5($("#txtChatPassword").val()));

                console.log(JSON.stringify(msg));

                g_aChatMsgs = msg["chatMsgs"];
                g_aChatMsgs.sort(chatMsgComparer);

                for(let n = 0; n < g_aChatMsgs.length; ++n) {

                    var strMsg = String(g_aChatMsgs[n].Message);
                    
                    if(strMsg.startsWith("#enc:") === true) {
                        
                        if(bEncrypt === true) {
                            strMsg = strMsg.substring(5);
                            strMsg = decryptString(String(strMsg), String(strKey));
                        };
                    };

                    strMsg = decodeURI(strMsg);
                    strMsg = strMsg.replaceAll("\n", "<br/>");

                    strHTML = strChatMsgEntry;
                    strHTML = strHTML.replace("{MSG}", strMsg);
                    strHTML = strHTML.replace("{TIME}", g_aChatMsgs[n].MsgTime);

                    if(g_aChatMsgs[n].Direction === 1) {

                        contact = getContact(g_aChatMsgs[n].ContactID);

                        if(contact !== null) {
                            strHTML = strHTML.replace("{USERNAME}", contact.Name + "@" + contact.Device);
                        }
                        else {
                            strHTML = strHTML.replace("{USERNAME}", "");
                        };

                        strHTML = strHTML.replace("{MSGCLASS}", "divChatMsgContainerIn");
                        strHTML = strHTML.replaceAll("{DIRCLASSLEFT}", "rowChatInHide");                    
                        strHTML = strHTML.replaceAll("{DIRCLASSRIGHT}", "rowChatIn");  
                        strHTML = strHTML.replaceAll("{MSGSTATE}", "MsgStateImageHidden");  
                        strHTML = strHTML.replaceAll("{MSGSTATEIMG}", "");
                    }
                    else {
                        strHTML = strHTML.replace("{USERNAME}", "You");
                        strHTML = strHTML.replace("{MSGCLASS}", "divChatMsgContainerOut");
                        strHTML = strHTML.replaceAll("{DIRCLASSLEFT}", "rowChatOut");
                        strHTML = strHTML.replaceAll("{DIRCLASSRIGHT}", "rowChatOutHide");
                        strHTML = strHTML.replaceAll("{MSGSTATE}", "MsgStateImage");
                        
                        if(g_aChatMsgs[n].TxComplete === 1) {
                            if(g_aChatMsgs[n].MsgRead === 1) {
                                strHTML = strHTML.replaceAll("{MSGSTATEIMG}", "./MsgRead.png");
                            }
                            else {
                                strHTML = strHTML.replaceAll("{MSGSTATEIMG}", "./LoRaGreen.png");
                            };
                        }
                        else {
                            strHTML = strHTML.replaceAll("{MSGSTATEIMG}", "./LoRaBlack.png");
                        };
                    };

                    strTemp += strHTML;
                };     

                $('#tbChatMsgsBody').html(strTemp);
                
                if(bResetNew === true) {
                    
                    if(chat.UnreadMsgs > 0) {

                        console.log("reset new msgs counter");


                        $.ajax({
                            url: g_strServer + 'api/api.json',
                            type: 'POST',
                            data: '{"command": "resetChatNewMsgCount", ' +
                                  ' "userID": ' + $("#hfUserID").val() + ', ' +
                                  ' "chatID": ' + chatID + ', ' +
                                  ' "hash": "' + $("#hfPwdHash").val() + '"}',
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
                                loadChatsFromDevice(true);
                            },
                            error: function (msg) {
                                console.log(JSON.stringify(msg));
                            }
                        });
                    };
                };
            },
            error: function (msg) {

                alert("Unable to load messages...");
                console.log(JSON.stringify(msg));

                $("#hfSelectedChatID").val("0");
            }
        });
    };
}



function checkContact(strContact) {
    //variables
    ///////////
    var oContact = getContactByString(strContact);
    
    if(oContact !== null) {
        if(oContact.Blocked === 1) {
            alert("You can't send a message to a blocked contact");
            $("#btnSendMessage").attr("disabled", true);
        };
    };
}


function sendMessage() {
    
    if(($("#txtMsgText").val().length > 0) && ($("#txtMsgTo").val().length > 0)) {
        $("#dlgWait").show();
        $("#lblDesc").text("Sending message...");
        
        var strMsg = encodeURI($("#txtMsgText").val());
        var strRcptTo = $("#txtMsgTo").val();
        
        if((strMsg.length < 1500) && (strRcptTo.length < 50)) {
            sendMessageToDevice(strRcptTo, strMsg, false, "");
        }
        else {
            $("#dlgWait").hide();
            
            alert("Message or receipient to long!");
        };
    };
};


function sendMessage2() {
    //variables
    ///////////
    var bEncrypt = $('#chkEncryptMessage').prop('checked');
    var strKey   = String(CryptoJS.MD5($("#txtChatPassword").val()));
    var strMsg   = encodeURI($("#txtMsgText2").val());
    
    if(strMsg.length > 0) {
        $("#dlgWait").show();
        $("#lblDesc").text("Sending message...");
        $("#btnSendMessage2").attr("disabled", true); 

        var chat        = getChatHeadByID($("#hfSelectedChatID").val());
        
        var strRcptTo   = chat.ChatRcpt;

        if((strMsg.length < 1500) && (strRcptTo.length < 50)) {
            
            if(chat !== null) {
                
                $("#txtMsgText2").val("");
                
                if(sendMessageToDevice(strRcptTo, strMsg, bEncrypt, strKey) === false) {
                    $("#btnSendMessage2").attr("disabled", false);
                };
            };
        }
        else {
            $("#dlgWait").hide();
            
            slert("Message or receipient to long!");
        };
    };
};



function sendMessageToDevice(strRcptTo, strMsg, bEncrypt, strKey) {
    
    if(bEncrypt === true) {
        if(strKey.length > 0) {
            strMsg = "#enc:" + encryptString(strMsg, strKey);
        }
        else {
            alert("Encryption enabled, but key not set, abort...!");
            
            $("#dlgWait").hide();
            
            return false;
        };
    };
        
    if((strMsg.length < 1500) && (strRcptTo.length < 50)){
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"chatcmd": "newChatMsg", ' +
                  ' "userID": ' + $("#hfUserID").val() + ', ' +
                  ' "rcptID": ' + "0" + ', ' +
                  ' "devID": ' + "0" + ', ' +
                  ' "contactID": ' + "0" + ', ' +
                  ' "rcptTo": "' + strRcptTo + '", ' +
                  ' "msg": "' + strMsg + '", ' +
                  ' "hash": "' + $("#hfPwdHash").val() + '"}',
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

                //wait for web event
            },
            error: function (msg) {

                alert("Unable to send message...");
                console.log(JSON.stringify(msg));

                $("#dlgWait").hide();
                $("#dlgNewChat").modal("hide");
                
                $("#btnSendMessage2").attr("disabled", false);
            }
        });
        
        return true;
    }
    else {
        alert("Message or recipient to long!");
        
        return false;
    };
}


function getChatHeadByID(chatID) {
    for(let x = 0; x < g_aChats.length; ++x) {
        if(parseInt(g_aChats[x].ID) === parseInt(chatID)) {
            return g_aChats[x];
        };
    };
    
    return null;
}



function showShoutOut() {
    $('#tbShoutOut').show(); 
    $('#tbChatMsgs').hide();
    $("#hfSelectedChatID").val("0");
    
    toggleMessageView(true);
    
    $("#txtMsgTextShoutOut").width($("#tbShoutOut").width() - 133);
    
    //load all shout out messages from the device,
    //the other messages will be loaded by the event 
    //handler
    if(g_bShoutoutLoaded === false) {
        loadShoutOutMsgs(0);
    };
};



function loadShoutOutMsgs(dwMsgID) {
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"command": "loadShoutOutMsgs", ' +
              ' "userID": ' + $("#hfUserID").val() + ', ' +
              (dwMsgID > 0 ? ' "EntryID": ' + dwMsgID + ', ' : '') +
              ' "hash": "' + $("#hfPwdHash").val() + '"}',
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
            //variables
            ///////////
            var strHTML = "";
            var strTemp = "";
            var msgs    = msg["shoutoutMsgs"];
            
            g_bShoutoutLoaded = true;
            
            console.log(JSON.stringify(msg));
            
            $("#txtMsgTextShoutOut").width($("#tbShoutOut").width() - 133);
            
            if(dwMsgID > 0) {
                strTemp = $('#tbShoutOutMsgsBody').html();
            };
            
            msgs.sort(shoutOutComparer);

            for(let n = 0; n < msgs.length; ++n) {
                
                var strMsg = String(msgs[n].Msg);

                strMsg = decodeURI(strMsg);
                
                strHTML = "<tr>";
                strHTML += "<td>" + msgs[n].SentTime + "</td>";
                strHTML += "<td>" + decodeURI(msgs[n].Sender) + "</td>";
                strHTML += "<td>" + strMsg + "</td>";
                strHTML += "</tr>";
                
                strTemp += strHTML;
            };     

            $('#tbShoutOutMsgsBody').html(strTemp);
        },
        error: function (msg) {

            alert("Unable to load shout out messages...");
            console.log(JSON.stringify(msg));
        }
    });
}



function sendMessageShoutOut() {
    //variables
    ///////////
    var strMsg    = encodeURI($("#txtMsgTextShoutOut").val());
    var strSender = $("#hfUser").val();
    
    if(strMsg.length <= 150) {
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"chatcmd": "newShoutOut", ' +
                  ' "userID": ' + $("#hfUserID").val() + ', ' +
                  ' "userName": "' + strSender + '", ' +
                  ' "msg": "' + strMsg + '", ' +
                  ' "hash": "' + $("#hfPwdHash").val() + '"}',
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

                //wait for web event
            },
            error: function (msg) {

                alert("Unable to send message...");
                console.log(JSON.stringify(msg));
            }
        });
    }
    else {
        $("#dlgWait").hide();
        
        alert("Message to long!");
    };
}



function blockContact(chatID, nBlockContact) {
    //variables
    ///////////
    var chat        = getChatHeadByID(chatID);
    var oContact    = null;
    
    if(chat !== null) {
        oContact = getContact(chat.ContactID);  
        
        if(oContact !== null) {
            $.ajax({
                url: g_strServer + 'api/api.json',
                type: 'POST',
                data: '{"command": "blockUserContact", ' +
                      ' "userID": ' + $("#hfUserID").val() + ', ' +
                      ' "contactID": ' + chat.ContactID + ', ' +
                      ' "blocked": ' + nBlockContact + ', ' +
                      ' "hash": "' + $("#hfPwdHash").val() + '"}',
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

                    oContact.Blocked = nBlockContact;

                    if(oContact.Blocked === 1) {
                        $("#txtMsgText2").attr("disabled", true);
                        $("#btnSendMessage2").attr("disabled", true);
                        $("#imgUnBlockContact").show();
                        $("#imgBlockContact").hide();
                    }
                    else {
                        $("#txtMsgText2").attr("disabled", false);
                        $("#btnSendMessage2").attr("disabled", false);
                        $("#imgUnBlockContact").hide();
                        $("#imgBlockContact").show();
                    };
                    
                    loadUserContacts(true);
                },
                error: function (msg) {

                    alert("Unable to block contact...");
                    console.log(JSON.stringify(msg));
                }
            });
        };
    };
};



function enableGpsTracking(nTrackingMode) {
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"chatcmd": "enablePositionTracking", ' +
              ' "userID": ' + $("#hfUserID").val() + ', ' +
              ' "type": ' + nTrackingMode + ', ' +
              ' "hash": "' + $("#hfPwdHash").val() + '"}',
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
        },
        error: function (msg) {

            alert("Unable to activate tracking...");
            console.log(JSON.stringify(msg));
        }
    });
}


function disableGpsTracking() {
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"chatcmd": "disablePositionTracking", ' +
              ' "userID": ' + $("#hfUserID").val() + ', ' +
              ' "hash": "' + $("#hfPwdHash").val() + '"}',
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
        },
        error: function (msg) {

            alert("Unable to activate tracking...");
            console.log(JSON.stringify(msg));
        }
    });
}



function showRadar() {
    //$("#divRadar").show();
    $("#dlgRadar").modal("show");
    $("#lblRadarRange").text(Math.round(g_pRadar.getRadius() / 2));
};


function CRadar(elRadar) {
    //variables
    ///////////
    const getCSSVal = (e, v) => e.style.getPropertyValue(v);
    const mod = (n, m) => ((n % m) + m) % m; // Fix negative Modulo
    const PI = Math.PI;
    const TAU = PI * 2;
    var   m_fOwnCourse      = 0;
    var   m_fMaxDistance    = 10;  //radius!
    var   m_aPoints         = [];
    var   m_bRangeFixed     = false;


    console.log("initRadar");
    
    
    document.getElementById("mulRadarRange").max = m_fMaxDistance / 2;



    CRadar.prototype.getRadius = function () {
        return m_fMaxDistance;
    };


    CRadar.prototype.getGpsData = function (pointID) {
        if(pointID in m_aPoints) {
            return m_aPoints[pointID];
        }
        else {
            return null;
        };
    };

    /*
     *             
     */
    CRadar.prototype.addPoint = function(gps) {
        //variables
        ///////////
        var strHTML = "";
        var fRadius = 0.0;
        var fCorrection = gps.Course - 90.0;
        var fCourse = 0;
        var _inst = this;
        var x = 0;
        var y = 0;

        //need to correct the course by 90 degree
        if(fCorrection < 0) {
            fCourse = 360 - fCorrection;
        }
        else {
            fCourse = fCorrection;
        };

        //remove old point, if shown in radar
        if(gps.Sender in m_aPoints) {
            if(document.getElementById("point_" + gps.Sender) !== null) {
                document.getElementById("point_" + gps.Sender).remove();
            };

            delete m_aPoints[gps.Sender];
        };


        //if the distance to the point is greater 
        //than the initial radius, increase the radius, 
        //if range is not set manually...
        if(gps.Dst > (m_fMaxDistance / 2)) {
            if(m_bRangeFixed === false) {
                m_fMaxDistance = (gps.Dst * 2.2); 

                document.getElementById("mulRadarRange").max = m_fMaxDistance / 2;
                
                //update all existing points
                m_aPoints.forEach(p => {
                    _inst.addPoint(p);
                });

                //update label
                $("#lblRadarRange").text(Math.round(m_fMaxDistance / 2));
                
                //update range slider
                document.getElementById("mulRadarRange").max = m_fMaxDistance / 2;
            }
            else {
                //update range slider
                document.getElementById("mulRadarRange").max = gps.Dst * 1.2;
            };
        };

        //add point
        m_aPoints[gps.Sender] = gps;


        //add the point only if in range...
        if(gps.Dst < (m_fMaxDistance / 2)) {
            
            //calculate the radius:
            //the coordinatesystem is 0, 0 for the top left corner 
            //and 1,1 for bottom right, so this must be aligned to
            //standard x/y coordinates, so 0,0 is 0.5,0.5... 
            //this means that fDistance must be alligned to 0 till .5
            //for the selected radius:
            fRadius = (0.5 / m_fMaxDistance) * gps.Dst;

            //calculate the position:
            let x = fRadius * Math.cos(fCourse * Math.PI / 180);
            let y = fRadius * Math.sin(fCourse * Math.PI / 180);

            x = (x + fRadius * Math.cos(fCourse * Math.PI / 180)) + 0.5;
            y = (y + fRadius * Math.sin(fCourse * Math.PI / 180)) + 0.5;

            strHTML = "<a id='point_" + gps.Sender + "' onclick=\"javascript: radarItemClicked('" + 
                    gps.Sender + "');\"><div class='" + (gps.Type === 0 ? "dotGreen" : "dotRed") + "' style='--x:" + x + 
                    "; --y:" + y + "'></div><div id='info_point_" + gps.Sender + "' class='dotBoxGreen' style='position: absolute;'><span>" + 
                    gps.Sender + "</span></div></a>";

            elRadar.innerHTML += strHTML;
        };
    };


    CRadar.prototype.setMaxRadius = function(value) {
        //variables
        ///////////
        var _inst       = this;
        var fMax        = 0;
        var _inst       = this;
        
        
        m_bRangeFixed  = true;
        m_fMaxDistance = parseInt(value) * 2;
        
        console.log("set max radius to: " + value);
        
        //get max from all device positions
        m_aPoints.forEach(p => {
            if(p.Dst > fMax) {
                fMax = p.Dst;
            };
        });
        
        if(parseInt(value) === 0) {
            console.log("Enable Auto range: " + fMax);
            
            if(fMax === 0) {
                fMax = parseInt(document.getElementById("mulRadarRange").max);
            };
            
            m_bRangeFixed  = false;
            m_fMaxDistance = fMax * 2;
        };
        
        //update label
        $("#lblRadarRange").text(Math.round(m_fMaxDistance / 2));
        
        //update points
        m_aPoints.forEach(p => {
            _inst.addPoint(p);
        });
    };


    CRadar.prototype.disableCourse = function(value) {
        //variables
        ///////////
        const elCourse = elRadar.querySelector(".course");
        
        console.log("set course disabled to: " + value);

        elCourse.style.display = (value === true ? "None" : "");
    };


    CRadar.prototype.setCourse = function(value) {
        
        console.log("New course: " + value);
        
        m_fOwnCourse = parseInt(value);
    };

    const update = () => {
        //variables
        ///////////
        const elCourse = elRadar.querySelector(".course");
        const elBeam = elRadar.querySelector(".beam");
        const beamAngle    = parseFloat(getComputedStyle(elBeam).getPropertyValue("rotate")) * PI / 180 || 0;

        //update own course 
        elCourse.style.transform = "rotate(" + m_fOwnCourse + "deg)";

        elsDot = elRadar.querySelectorAll(".dotGreen, .dotRed");

        elsDot.forEach(elDot => {
            const x = getCSSVal(elDot, "--x") - 0.5;
            const y = getCSSVal(elDot, "--y") - 0.5;
            const dotAngle = mod(Math.atan2(y, x), TAU);
            const opacity = mod(dotAngle - beamAngle, TAU) / TAU;
            elDot.style.opacity = opacity;

            var rect = elDot.getBoundingClientRect();
            var infoDiv = document.getElementById("info_" + elDot.parentElement.id);

            infoDiv.style.top = (rect.top + 2) + 'px';
            infoDiv.style.left = (rect.left - 8) + 'px';
        });

        requestAnimationFrame(update);
    };

    update();
};


function getNodeByID(nodeID) {
    for(var i = 0; i < g_aKnownNodes.length; ++i) {
        if(parseInt(g_aKnownNodes[i].NodeID) === nodeID) {
            return g_aKnownNodes[i];
        };
    };
    
    return null;
}

function radarItemClicked(pointID) {
    //variables
    ///////////
    var oNode = getNodeByID(parseInt(pointID));
    var oGPS = g_pRadar.getGpsData(pointID);
    var strHTML = "";
    
    $("#tbNodeDetailsBody").empty();
    
    if(oNode !== null) {
        strHTML = "<tr><td>" + pointID + "</td><td>" + oNode.DevName + "</td><td>" + oGPS.Lat + "</td><td>" + oGPS.Lon + "</td><td>" + oGPS.Speed + "</td><td>" + oGPS.Course + "</td><td>" + oGPS.Dst + "</td></tr>";
        
        $("#tbNodeDetailsBody").html(strHTML);
    };
};