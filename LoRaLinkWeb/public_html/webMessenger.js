//globals
/////////
var g_dwNodeID        = 0;
var g_nDeviceType     = 1;      //personal by default
var g_nAppMinWidth    = 900;
var g_strServer       = "/";
var g_nLoadPos        = 0;
var MAX_MESSAGE_CHARS = 1500;
var g_aKnownNodes     = [];
var g_aRoutes         = [];
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
var g_fConfigLat      = 0;
var g_fConfigLon      = 0;
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
var g_pMap            = null;
var g_pMarkerMe       = null;
var g_pNodeMarkers    = [];
var g_aTileDownloader = [];
var g_aTileCheck      = [];
var g_bTileDownload   = false;
var g_bOnlineTiles    = false;
var g_bPoisChanged    = false;
var g_bRecordTrack    = false;
var g_bWaitingEvent   = false;


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
        //when the browser has connectivity issues, or runs in background,
        //i saw, that the timer was still raised, but the callback function
        //was not called...
        //this resulted in hunderds of queries, when the dev came back, or 
        //the browser got back to front...
        //so, this should avoid sending new requests, when the other req was 
        //not finihed, but removes the timer event...
        if(g_bWaitingEvent === false) {
            
            g_bWaitingEvent = true;
            
            readWebEvents();

            //dwonload tiles one by one...
            //had to also use a bool to avoid async downloading of tiles, which 
            //crashed the esp (even if async set to false, i had multiple uploads)
            //on tbeam check & upload freezed the webserver process, so download
            //will start after the tiles are checked...
            if((g_bTileDownload === false) && (g_bOnlineTiles === true) && (g_aTileCheck.length <= 0)) { 

                if($("#pbTileDownloader").hasClass("bg-success") === false) {
                    $("#pbTileDownloader").addClass("bg-success");
                    $("#pbTileDownloader").removeClass("bg-warning");

                    //reset progress bar
                    $("#pbTileDownloader").attr("aria-valuenow", "0");
                    $("#pbTileDownloader").attr("aria-valuemax", g_aTileDownloader.length);
                    $("#pbTileDownloader").css("width", "0%");
                }
                else {
                    if(parseInt($("#pbTileDownloader").attr("aria-valuemax")) < g_aTileDownloader.length) {
                        $("#pbTileDownloader").attr("aria-valuemax", g_aTileDownloader.length);
                        $("#pbTileDownloader").attr("aria-valuenow", "0");
                    };
                };

                tileDownloader();
            };

            //check if tiles exist
            if(g_bOnlineTiles === true) {

                if(g_aTileCheck.length > 0) {
                    if($("#pbTileDownloader").hasClass("bg-success") === true) {
                        $("#pbTileDownloader").removeClass("bg-success");
                        $("#pbTileDownloader").addClass("bg-warning");

                        //reset progress bar
                        $("#pbTileDownloader").attr("aria-valuenow", "0");
                        $("#pbTileDownloader").attr("aria-valuemax", g_aTileCheck.length);
                        $("#pbTileDownloader").css("width", "0%");
                    }
                    else {
                        if(parseInt($("#pbTileDownloader").attr("aria-valuemax")) < g_aTileCheck.length) {
                            $("#pbTileDownloader").attr("aria-valuemax", g_aTileCheck.length);
                            $("#pbTileDownloader").attr("aria-valuenow", "0");
                        };
                    };

                    tileCheck();
                };
            };
            
            g_bWaitingEvent = false;
        };
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
        async: false,
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
                    
                    if(g_bGpsValid === false) {
                        $("#imgGpsState").attr("src", "validGPS.png");
                    };

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

                    if(g_bGpsValid === true) {
                        $("#imgGpsState").attr("src", "invalidGPS.png");
                    };
                };

                g_bGpsValid = msg["bValidSignal"];
                g_fLocalLat = msg["fLatitude"];
                g_fLocalLon = msg["fLongitude"];
                
                if((msg["bTrackingActive"] !== g_bTrackingActive) || (g_nTrackingType !== msg["nTrackingType"])) {
                    if(msg["bTrackingActive"] === true) {
                        
                        $("#imgLocationTracking").show();
                        
                        if($("#mnuDisableGpsTracking").hasClass("disabled") == true) {
                            $("#mnuDisableGpsTracking").removeClass("disabled");
                        };
                        
                        if($("#mnuEnableEmergGpsTracking").hasClass("disabled") == false) {
                            $("#mnuEnableEmergGpsTracking").addClass("disabled");
                        };
                        
                        if($("#mnuEnableGpsTracking").hasClass("disabled") == false) {
                            $("#mnuEnableGpsTracking").addClass("disabled");
                        };
                        
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
                        
                        if($("#mnuDisableGpsTracking").hasClass("disabled") == false) {
                            $("#mnuDisableGpsTracking").addClass("disabled");
                        };
                        
                        if($("#mnuEnableEmergGpsTracking").hasClass("disabled") == true) {
                            $("#mnuEnableEmergGpsTracking").removeClass("disabled");
                        };
                        
                        if($("#mnuEnableGpsTracking").hasClass("disabled") == true) {
                            $("#mnuEnableGpsTracking").removeClass("disabled");
                        };
                    };
                    
                    g_bTrackingActive = msg["bTrackingActive"];
                };
                
                
                if(g_bRecordTrack !== msg["bRecordTrack"]) {
                    g_bRecordTrack = msg["bRecordTrack"];
                    
                    if(g_bRecordTrack === true) {
                        if($("#mnuRecordTrackStop").hasClass("disabled") == true) {
                            $("#mnuRecordTrackStop").removeClass("disabled");
                        };
                        
                        if($("#mnuRecordTrack").hasClass("disabled") == false) {
                            $("#mnuRecordTrack").addClass("disabled");
                        };
                    }
                    else {
                        if($("#mnuRecordTrackStop").hasClass("disabled") == false) {
                            $("#mnuRecordTrackStop").addClass("disabled");
                        };
                        
                        if($("#mnuRecordTrack").hasClass("disabled") == true) {
                            $("#mnuRecordTrack").removeClass("disabled");
                        };
                    };
                };
                
                //update my pos on map
                if(g_pMarkerMe !== null) {
                    var newLatLng = new L.LatLng(g_fLocalLat, g_fLocalLon);
                    g_pMarkerMe.setLatLng(newLatLng); 
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
                         
                        for(var n = 0; n < g_pNodeMarkers.length; ++n) {
                            if(g_pNodeMarkers[n].key === gps.Sender.toString()) {
                                var newLatLng = new L.LatLng(parseFloat(gps.Lat), parseFloat(gps.Lon));
                    
                                g_pNodeMarkers[n].value.Marker.setLatLng(newLatLng); 
                                
                                break;
                            };
                        };
                    };
                };
                break;
            };
        },
        error: function (msg) {

            console.log("Unable to read events");
            console.log(JSON.stringify(msg));
            
            if(g_bConnected === true) {
                g_bConnected = false;

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


function showMapView() {
    
    toggleMessageView(true);
    
    $("#tbChatMsgs").hide();
    $("#tbShoutOut").hide();
    
    $("#divMapContainer").html("<div id=\"map\"></div>");
    $("#divMapContainer").show();
    
    //ask dev if it is connected to a wifi network
    //if true, assume, that the user connects via the 
    //external wifi connection, so that the client can 
    //access the web, if he connects via the dev ap, this 
    //won't work...
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "GetWiFi"' +
              '}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            console.log(JSON.stringify(msg));
            
            if(msg["response"] === "OK") {
                if((msg["szDevIP"] !== "Not Connected") && (msg["szDevIP"].length > 0)) {
                    
                    console.log("Show online data");
                    g_bOnlineTiles = true;
                    
                    if(g_bGpsValid === true) {
                        // Create the map
                        g_pMap = L.map('map').setView([g_fLocalLat, g_fLocalLon], 4);
                    }
                    else {
                        // Create the map
                        g_pMap = L.map('map').setView([51, 6], 4);
                    };

                    // Set up the OSM layer
                    L.tileLayer(
                    'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png'
                    ).addTo(g_pMap);
                }
                else {
                    console.log("Show offline data");
                    g_bOnlineTiles = false;
                    
                    if(g_bGpsValid === true) {
                        // Create the map
                        g_pMap = L.map('map').setView([g_fLocalLat, g_fLocalLon], 4);
                    }
                    else {
                        // Create the map
                        g_pMap = L.map('map').setView([51, 6], 4);
                    };
                    
                    // Set up the OSM layer
                    L.tileLayer(
                    '/tiles/{z}/{x}/{y}.png'
                    ).addTo(g_pMap);
                };
                
                
                //create back btn
                var btnMapBack = L.control({position: 'topright'});

                btnMapBack.onAdd = function (g_pMap) {

                    var div = L.DomUtil.create('div', '');
                    
                    div.innerHTML += "<a onclick='javascript: closeMapView();'><img src='/back.png' width='40' height='40'></a>";
                    
                    return div;
                };
                
                btnMapBack.addTo(g_pMap);
                
                
                //show device labels
                loadKnownDevices(true, true);
                
                
                //show pois on map
                addPoisOnMap();
                
                //add on click handler which adds the last clicked coordinates to the 
                //manage poi dialog
                g_pMap.on('click', function(e){
                    var coord = e.latlng;
                    var lat = coord.lat;
                    var lng = coord.lng;
                    
                    console.log("clicked the map at latitude: " + lat + " and longitude: " + lng);
                    
                    $("#txtPoiLatitude").val(lat);
                    $("#txtPoiLongitude").val(lng);
                });
            };
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));
        }
    });
};



function closeMapView() {
    //reset markers
    g_pMarkerMe = null;
    g_pMap      = null;
    
    $("#divMapContainer").hide();
    toggleMessageView(false);
}


/**
 * whooha, ai generated code, lazy stuff... :D 
 * 
 * @param {type} longitude
 * @param {type} latitude
 * @returns {String}
 */
function convertToLocator(longitude, latitude) {
    const longitudeDegrees = Math.floor((longitude + 180) / 20) + 1;
    const latitudeDegrees = Math.floor((latitude + 90) / 10) + 1;
    const longitudeMinutes = Math.floor(((longitude + 180) % 20) * 60 / 2.5) % 24;
    const latitudeMinutes = Math.floor(((latitude + 90) % 10) * 60 / 1.5) % 24;
    const longitudeSeconds = Math.floor((((longitude + 180) % 20) * 60 / 2.5) % 1) * 60;
    const latitudeSeconds = Math.floor((((latitude + 90) % 10) * 60 / 1.5) % 1) * 60;
    return String.fromCharCode(65 + longitudeDegrees - 1) + String.fromCharCode(65 + latitudeDegrees - 1) + longitudeMinutes.toString().padStart(2, '0') + latitudeMinutes.toString().padStart(2, '0') + String.fromCharCode(97 + longitudeSeconds) + String.fromCharCode(97 + latitudeSeconds);
};


/**
 * this function shows the device markers on the map
 * 
 * @returns {undefined}
 */
function updateDeviceMarkers() {
    //variables
    ///////////
    var blueIcon = new L.icon({iconUrl: '/images/marker-icon.png',
        shadowUrl: '/images/marker-shadow.png'
        });
    var greenIcon = new L.icon({iconUrl: '/images/marker-icon-green.png',
        shadowUrl: '/images/marker-shadow.png'
    });
    var oNodeMarker = {};
    
    
    g_pNodeMarkers = [];
    
    for(var i = 0; i < g_aKnownNodes.length; ++i) {
        //if pos is valid
        if((parseFloat(g_aKnownNodes[i].posN) !== 0) && (parseFloat(g_aKnownNodes[i].posE) !== 0)) {
            
            oNodeMarker = {};
            oNodeMarker.NodeID = g_aKnownNodes[i].NodeID;
            oNodeMarker.Marker = L.marker([parseFloat(g_aKnownNodes[i].posN), parseFloat(g_aKnownNodes[i].posE)], {icon: blueIcon});
            
            oNodeMarker.Marker.addTo(g_pMap).bindPopup(
                    "<b>" + g_aKnownNodes[i].DevName + "</b><br/>ID: " + 
                    g_aKnownNodes[i].NodeID + "<br/>Last Heard: " + g_aKnownNodes[i].LastHeard + "<br/>WGS84: " +
                    g_aKnownNodes[i].posN + ", " + g_aKnownNodes[i].posE + "<br/>Loc: " +
                    convertToLocator(g_aKnownNodes[i].posE, g_aKnownNodes[i].posN)
            );
    
            g_pNodeMarkers.push({key: oNodeMarker.NodeID, value: oNodeMarker});
        };
    };
    
    
    if(g_bGpsValid === true) {
        if((parseFloat(g_fLocalLat) !== 0) && (parseFloat(g_fLocalLon) !== 0)) {
            
            g_pMarkerMe = L.marker([parseFloat(g_fLocalLat), parseFloat(g_fLocalLon)], {icon: greenIcon});
           
            g_pMarkerMe.addTo(g_pMap).bindPopup(
                "<b>" + $("#hfNodeName").val() + " (Me)</b><br/>ID: " + 
                g_dwNodeID + "<br/>WGS84: " +
                g_fLocalLat + ", " + g_fLocalLon + "<br/>Loc: " +
                convertToLocator(g_fLocalLon, g_fLocalLat)
            );
        };
    }
    else {
        g_pMarkerMe = null;
        
        //add only if not set to 0/0
        if((g_fConfigLat != 0) && (g_fConfigLon !== 0)) {
            if((parseFloat(g_fConfigLat) !== 0) && (parseFloat(g_fConfigLon) !== 0)) {
                L.marker([parseFloat(g_fConfigLat), parseFloat(g_fConfigLon)], {icon: greenIcon}).addTo(g_pMap).bindPopup(
                    "<b>" + $("#hfNodeName").val() + " (Me)</b><br/>ID: " + 
                    g_dwNodeID + "<br/>WGS84: " +
                    g_fConfigLat + ", " + g_fConfigLon + "<br/>Loc: " +
                    convertToLocator(g_fConfigLon, g_fConfigLat) + "<br/>Config value"
                );
            };
        }
    }
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



/**
 * load device config, user settings and messages
 * 
 * @returns {undefined}
 */
function onLoad() {
    //variables
    ///////////
    var QueryString = getRequests();
    
    $("#hfPwdHash").val(QueryString["pwd"]);
    $("#hfUser").val(QueryString["user"]);
    $("#hfUserID").val(QueryString["userid"]);
    $("#hfSelectedChatID").val("0");
  
    //check if query string had all parameters
    if(($("#hfUserID").val().length <= 0) || ($("#hfUser").val().length <= 0) || ($("#hfPwdHash").val().length <= 0)) {
        window.location.href = "./index.html";
    }
    else {
        resizeWindow();
        
        
        //get device config
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"command": "GetDeviceCfg", ' +
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

                if(msg["response"] === "OK") {
                    $("#hfNodeName").val(msg["szDevName"]);
                    $("#lblNodeNme").text(msg["szDevName"]);
                    
                    g_dwNodeID      = msg["dwDeviceID"];
                    g_nDeviceType   = msg["nDeviceType"];
                    g_fConfigLat    = msg["fLocN"];
                    g_fConfigLon    = msg["fLocE"];
                };
            },
            error: function (msg) {
                console.log(JSON.stringify(msg));
                
                alert("Failed to load device config!");
                
                window.location.href = "./index.html";
            }
        });
        
        
        //add eventhandler to capture tile requests to store OSM 
        //tiles on the device...
        document.addEventListener("tileLoaded", function(e) {
            
            if(g_bOnlineTiles == true) {
                console.log("event src: " + e.detail); 

                if(g_aTileCheck.indexOf(e.detail) === -1) {
                    g_aTileCheck.push(e.detail);
                };
            };
        });
        

        loadUserContacts(false);
        
        initWebEventReader();
    };
};



/**
 * this function checks visited tiles if they exist on the device.
 * If they are not found, they will be added to the downloader
 * 
 * @returns {Number}
 */
async function tileCheck() {
    //variables
    ///////////
    var strFile = "";

    if(g_aTileCheck.length > 0) {
        
        console.log("tileCheck: check tile: " + g_aTileCheck[0]);
        
        //get the image path 
        //since the source is openstreetmap, search for .org
        strFile = g_aTileCheck[0].substring(g_aTileCheck[0].indexOf(".org") + 4);

        console.log("file: " + strFile);

        //check if tile exist on the device
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"command": "fileExist", ' +
                  ' "file": "' + "/tiles" + strFile + '"' +
                  '}',
            contentType: 'application/json; charset=utf-8',
            crossDomain: true,
            dataType: 'json',
            async: false,
            headers: {
                "accept": "application/json",
                "Access-Control-Allow-Origin": "*",
                "Access-Control-Allow-Headers": "Content-Type, Accept, x-requested-with, x-requested-by",
                "Access-Control-Allow-Methods": "GET, POST"
            },
            success: async function(msg) {
                
                console.log(JSON.stringify(msg));
                
                if(msg["response"] === "ERR") {
                    if(g_aTileDownloader.indexOf(g_aTileCheck[0]) === -1) {
                        g_aTileDownloader.push(g_aTileCheck[0]);
                    };        
                };
            },
            error: function (msg) {

                console.log(JSON.stringify(msg));
            }
        });
        
        g_aTileCheck.splice(0, 1);
        
        //update progress bar
        $("#pbTileDownloader").attr("aria-valuenow", parseInt($("#pbTileDownloader").attr("aria-valuenow")) + 1);
        $("#pbTileDownloader").css("width", ((parseInt($("#pbTileDownloader").attr("aria-valuenow")) / g_aTileCheck.length) * 100.0) + "%");
    };
     
    return g_aTileCheck.length;
};



/**
 * this function downloads visited tiles to the sd card one by one.
 * the downloader will be called once a second to avoid flooding the 
 * device with requests.
 * 
 * @returns {Number}
 */
async function tileDownloader() {
    //variables
    ///////////
    var strFile = "";
    var strPath = "";
    var strFolder = "";
    var nIdx = 0;
    var bErr = false;
    
    
    if(g_bTileDownload === false) {
        if(g_aTileDownloader.length > 0) {
            console.log("tileDownloader: download tile: " + g_aTileDownloader[0]);
            
            g_bTileDownload = true;
            
            //get the image path 
            //since the source is openstreetmap, search for .org
            strFile = g_aTileDownloader[0].substring(g_aTileDownloader[0].indexOf(".org") + 4);

            //try to create the path, don't care if it exist...
            strFile = "/tiles" + strFile;

            while((nIdx < strFile.length) && (bErr === false)) {
                strPath = strFile.substring(0, strFile.indexOf("/", nIdx + 1));
                nIdx    = strFile.indexOf("/", nIdx + 1);

                if(nIdx < 0) {
                    break;
                };

                if(strPath !== "/tiles") {

                    strFolder = strPath;
                    console.log("Create dir: " + strPath);

                    $.ajax({
                        url: g_strServer + 'api/api.json',
                        type: 'POST',
                        crossDomain: true,
                        data: '{"command": "createFolder", ' +
                              ' "NewFolder": "' + strPath + '", ' +
                              ' "Folder": ""}',
                        contentType: 'application/json; charset=utf-8',
                        dataType: 'json',
                        async: false,
                        success: function(msg) {

                        },
                        error: function (msg) {
                            console.log("Error create folder:");

                            console.log(JSON.stringify(msg));
                            
                            g_bTileDownload = false;
                            bErr            = true;
                        }
                    });                    
                };
            };

            if(bErr === false) {
                //path should exist now, download the file
                const res = await downloadImage(strFolder, strFile, g_aTileDownloader[0]);

                g_aTileDownloader.splice(0, 1);
            };
            
            g_bTileDownload = false;
        }
        else {
            $("#pbTileDownloader").attr("aria-valuenow", "0");
            $("#pbTileDownloader").attr("aria-valuemax", "0");
            $("#pbTileDownloader").css("width", "0%");
            
            g_bTileDownload = false;
        };
    };
    
    return g_aTileDownloader.length;
};


/**
 * this function downloads the image, converts the downloaded blob to 
 * a file, passes it to the upload form and calls the upload function 
 * 
 * @param {type} strPath
 * @param {type} strFile
 * @param {type} imageSrc
 * @returns {unresolved}
 */
async function downloadImage(strPath, strFile, imageSrc) {
    //variables
    ///////////
    const form          = document.getElementById("frmFileUpload");
    const fileInput     = document.getElementById('file');
    const dataTransfer  = new DataTransfer();
    const image         = await fetch(imageSrc);
    const imageBlob     = await image.blob();
    var strFileName     = strFile.replace(strPath, "");
    const file          = new File([imageBlob], strFileName.substring(1), { type: imageBlob.type });
    
  
    console.log("Save data from " + imageSrc + " to folder: " + strPath + " file: " + strFileName.substring(1));
    
    dataTransfer.items.add(file);
    fileInput.files = dataTransfer.files;
    
    var fd = new FormData(form);
    const res = await uploadFile(fd, strPath); 

    //update progress bar
    $("#pbTileDownloader").attr("aria-valuenow", parseInt($("#pbTileDownloader").attr("aria-valuenow")) + 1);
    $("#pbTileDownloader").css("width", ((parseInt($("#pbTileDownloader").attr("aria-valuenow")) / g_aTileDownloader.length) * 100) + "%");
    
    g_bTileDownload = false; 
    
    return res;
};


/**
 * this function sends the file to the device. this function will be used 
 * for tile uploading and storing data which are not managed by the device
 * (client side functions like poi's)
 * 
 * @param {type} formData
 * @param {type} strPath
 * @returns {unresolved}
 */
async function uploadFile(formData, strPath) {
    return fetch("/upload?folder=" + encodeURI(strPath),
        {
          method: 'POST',
          body: formData
        }
    )
    .then((response)=> { return response; });
}



$(window).on("resize", function() {
    resizeWindow();
});


function loadFinished() {
    console.log("load finished...");
    
    //hide after load
    $("#dlgWait").hide();
}



function showNewChatDialog(strRcpt) {
    $("#txtMsgTo").val(strRcpt);
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
                loadKnownDevices(false, false);
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


function loadKnownDevices(bUpdateOnly, bCreateMapLabels) {
    
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
            
            if(bUpdateOnly == false) {
                loadFinished();
            };
            
            if(bCreateMapLabels == true) {
                updateDeviceMarkers();
            };
        },
        error: function (msg) {

            console.log(JSON.stringify(msg));

            if(bUpdateOnly == false) {
                loadFinished();
            };
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
    
    
    $("#divMapContainer").hide();
    
    
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
        }
        else {
            $("#txtMsgText2").attr("disabled", false);
            $("#btnSendMessage2").attr("disabled", false);
            $("#imgUnBlockContact").hide();
            $("#imgBlockContact").show();
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
    $("#divMapContainer").hide();
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
                var strSenderDev = decodeURI(msgs[n].Sender).substr(decodeURI(msgs[n].Sender).indexOf("@") + 1);
                
                        
                $("#hfNodeName").val()

                strMsg = decodeURI(strMsg);
                
                strHTML = "<tr>";
                strHTML += "<td style='width: 120px;'>" + msgs[n].SentTime + "</td>";
                
                if($("#hfNodeName").val() !== strSenderDev) {
                    strHTML += "<td style='width: 150px; overflow: hidden;'><a onclick='javascript: showNewChatDialog(\"" + decodeURI(msgs[n].Sender) + "\");'>" + decodeURI(msgs[n].Sender) + "</a></td>";
                }
                else {
                    strHTML += "<td style='width: 150px; overflow: hidden;'><span>" + decodeURI(msgs[n].Sender) + "</span></td>";
                };
                
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


function getRouting() {
    
    $("#lblDesc").text("Get routing...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "getRoutes"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            
            g_aRoutes = [];
            
            for(var i = 0; i < msg["Routes"].length; ++i) {
                var oRoute = {};
                
                oRoute.DeviceID = parseInt(msg["Routes"][i][0]);
                oRoute.ViaNodeID = parseInt(msg["Routes"][i][1]);
                oRoute.ConnType = parseInt(msg["Routes"][i][2]);
                
                g_aRoutes.push({key: oRoute.DeviceID, value: oRoute});
            }
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load routes...");
        }
    });
};



function showRadar() {
    
    loadKnownDevices(true, false);
    getRouting();
    
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
                    "; --y:" + y + ";'></div><div id='info_point_" + gps.Sender + "' class='dotBoxGreen' style='position: absolute; --x:" + x + 
                    "; --y:" + y + ";'><span>" + 
                    gps.Sender + "</span></div></a>";

            elRadar.innerHTML += strHTML;
        };
    };


    CRadar.prototype.getPoint = function(pointID) {
        if(pointID in m_aPoints) {
            return m_aPoints[pointID];
        };
        
        return null;
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


//poi management
////////////////



/**
 * this function downloads the poidata.json file from the device and
 * add the data to the table. POis are outside the device functionallity,
 * internally they will not be handled, so handling of POIs are a client 
 * side feature...
 * 
 * @returns {undefined}
 */
function showManagePOI() {
    //variables
    ///////////
    var strHTML = "";
    
    $("#tbPoiDataBody").html("");
    $("#dlgPoiManagement").modal("show");
    
    fetch('/poidata/' + $("#hfUserID").val() + '.json', {
        method: 'GET',
        headers: {
            'Accept': 'application/json'
        }
    })
   .then(response => response.json())
   .then(response => { 
        console.log(JSON.stringify(response));
       
        for(var n = 0; n < response["pois"].length; ++n) {
            strHTML += "<tr id='poidata_" + n + "'>";
            strHTML += "    <td><span>" + response["pois"][n].Latitude + "</span></td>";
            strHTML += "    <td><span>" + response["pois"][n].Longitude + "</span></td>";
            strHTML += "    <td><img src='" + response["pois"][n].Icon + "' style='height: 15px;'></td>";
            strHTML += "    <td><span>" + decodeURI(response["pois"][n].Desc) + "</span></td>";
            strHTML += "    <td><a onclick='javascript: document.getElementById(\"poidata_" + n + "\").remove(); g_bPoisChanged = true;'>delete</a></td>";
            strHTML += "</tr>";
        };
        
        $("#tbPoiDataBody").html(strHTML);
   });
   
   g_bPoisChanged = false;
};


/**
 * this function adds the pois to the map
 * 
 * @returns {undefined}
 */
function addPoisOnMap() {
    
    fetch('/poidata/' + $("#hfUserID").val() + '.json', {
        method: 'GET',
        headers: {
            'Accept': 'application/json'
        }
    })
   .then(response => response.json())
   .then(response => { 
        console.log(JSON.stringify(response));
       
        for(var n = 0; n < response["pois"].length; ++n) {

            var mapicon = new L.icon({iconUrl: response["pois"][n].Icon,
                shadowUrl: '/images/marker-shadow.png'
            });
    
            L.marker([parseFloat(response["pois"][n].Latitude), parseFloat(response["pois"][n].Longitude)], {icon: mapicon}).addTo(g_pMap).bindPopup(
                "<b>POI:</b><br/>" + decodeURI(response["pois"][n].Desc) + "<br/>WGS84: " +
                response["pois"][n].Latitude + ", " + response["pois"][n].Longitude + "<br/>Loc: " +
                convertToLocator(response["pois"][n].Longitude, response["pois"][n].Latitude));
        };
   });
};




/**
 * this function adds a poi to the table
 * 
 * @returns {undefined}
 */
function addPoi() {
    //variables
    ///////////
    var element = document.getElementById("tbPoiDataBody");
    var number = element.getElementsByTagName('*').length + 1;
    var strHTML = $("#tbPoiDataBody").html();
    
    if(($("#txtPoiLatitude").val().length > 0) && ($("#txtPoiLongitude").val().length > 0) && ($('#cmbPoiIcon').attr('data-selected').length > 0)) {
        strHTML += "<tr id='poidata_" + number + "'>";
        strHTML += "    <td><span>" + $("#txtPoiLatitude").val() + "</span></td>";
        strHTML += "    <td><span>" + $("#txtPoiLongitude").val() + "</span></td>";
        strHTML += "    <td><img src='" + $('#cmbPoiIcon').attr('data-selected') + "' style='height: 15px;'></td>";
        strHTML += "    <td><span>" + $("#txtPoiDesc").val() + "</span></td>";
        strHTML += "    <td><a onclick='javascript: document.getElementById(\"poidata_" + number + "\").remove();'>delete</a></td>";
        strHTML += "</tr>";
        
        $("#tbPoiDataBody").html(strHTML);
    };
    
    g_bPoisChanged = true;
};



/**
 * when the poi management dialog was closed, this function uploads
 * the pois to the device, when the data was modified...
 * 
 * @returns {undefined}
 */
async function savePoiData() {
    //variables
    ///////////
    const form          = document.getElementById("frmFileUpload");
    const fileInput     = document.getElementById('file');
    const dataTransfer  = new DataTransfer();
    var element         = document.getElementById("tbPoiDataBody");
    var number          = element.getElementsByTagName('*').length;
    var strJson         = "{\"pois\": [";
    
    if(g_bPoisChanged === true) {
        if(number > 0) {
            for(var r = 0; r < element.rows.length; ++r) {
                if(r > 0) {
                    strJson += ",";
                };

                strJson += "{";

                strJson += "\"Latitude\": " + $(element.rows[r].cells[0]).children('span').first().text() + ", ";
                strJson += "\"Longitude\": " + $(element.rows[r].cells[1]).children('span').first().text() + ", ";
                strJson += "\"Icon\": \"" + $(element.rows[r].cells[2]).children('img').first().attr("src") + "\", ";
                strJson += "\"Desc\": \"" + encodeURI($(element.rows[r].cells[3]).children('span').first().text()) + "\"";

                strJson += "}";
            }; 
        };

        strJson += "]}";


        var oMyBlob = new Blob([strJson], {type : 'application/json'});
        var file    = new File([oMyBlob], $("#hfUserID").val() + '.json', { type: oMyBlob.type });

        dataTransfer.items.add(file);
        fileInput.files = dataTransfer.files;

        var fd      = new FormData(form);
        const res   = await uploadFile(fd, "/poidata");
    };
};



/**
 * this function exports the pois as json file
 * 
 * @returns {undefined}
 */
function exportPOI() {
    fetch('/poidata/' + $("#hfUserID").val() + '.json', {
        method: 'GET',
        headers: {
            'Accept': 'application/json'
        }
    })
    .then(response => response.json())
    .then(response => { 
        var strJson = JSON.stringify(response);

        console.log(strJson);

        const file = new File([strJson], 'pois.json', {
            type: 'text/plain'
        });

        downloadFileContent(file);
   });
};



function importPOI() {
    $("#fOpenFileDialog").val("");
    $("#fOpenFileDialog").click();
};


var openFile = function(event) {
    var input = event.target;
    var reader = new FileReader();
    var element = document.getElementById("tbPoiDataBody");
    var number = element.getElementsByTagName('*').length + 1;
    
    reader.onload = function() {
        //variables
        ///////////
        var text = reader.result;
        var jsn  = JSON.parse(text);
        var strHTML = $("#tbPoiDataBody").html();
        
        for(var n = 0; n < jsn["pois"].length; ++n) {
            strHTML += "<tr id='poidata_" + (n + number) + "'>";
            strHTML += "    <td><span>" + jsn["pois"][n].Latitude + "</span></td>";
            strHTML += "    <td><span>" + jsn["pois"][n].Longitude + "</span></td>";
            strHTML += "    <td><img src='" + jsn["pois"][n].Icon + "' style='height: 15px;'></td>";
            strHTML += "    <td><span>" + decodeURI(jsn["pois"][n].Desc) + "</span></td>";
            strHTML += "    <td><a onclick='javascript: document.getElementById(\"poidata_" + (n + number) + "\").remove(); g_bPoisChanged = true;'>delete</a></td>";
            strHTML += "</tr>";
            
            g_bPoisChanged = true;
        };
        
        $("#tbPoiDataBody").html(strHTML);
    };

    reader.readAsText(input.files[0]);
};



//gps track handling
////////////////////


function showTrackRecordDialog() {
    $("#dlgNewTrackRecord").modal("show");
};


function startTrackRecording() {
    if(encodeURI($("#txtTrackName").val()).length < 255) {
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"chatcmd": "startTrackRecord", ' +
                  ' "userID": ' + $("#hfUserID").val() + ', ' +
                  ' "desc": "' + encodeURI($("#txtTrackName").val()) + '", ' +
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

                alert("Unable to activate recording...");
                console.log(JSON.stringify(msg));
            }
        });
        
        $("#dlgNewTrackRecord").modal("hide");
    }
    else {
        alert("Track name to long!");
    };
};



function stopTrackRecording() {

    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"chatcmd": "stopTrackRecord", ' +
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

            alert("Unable to stop recording...");
            console.log(JSON.stringify(msg));
        }
    });
};


function showTracks() {
    $("#dlgTrackManagement").modal("show");
    $("#tbTrackDataBody").html("");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"chatcmd": "getTracks", ' +
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
            var strHTML = "";
            
            console.log(JSON.stringify(msg));
            
            for(var i = 0; i < msg["Tracks"].length; ++i) {
                strHTML += "<tr><td><span>" + msg["Tracks"][i].Time + "</span></td><td><span>" + decodeURI(msg["Tracks"][i].Desc) + (msg["Tracks"][i].Active === 1 ? " (recording)" : "") + "</span></td>" +
                        "<td><a onclick='javascript: exportTrack(" + msg["Tracks"][i].ID + ", \"" + decodeURI(msg["Tracks"][i].Desc) + "\");'><img style='margin-left: 12px; height: 20px;' src='/images/disk.png' alt='export track'></a></td>" +
                        "<td><a onclick='javascript: deleteTrack(" + msg["Tracks"][i].ID + ");'><img style='margin-left: 12px; height: 20px;' src='/images/trash.png' alt='delete track'></a></td>" +
                        "<td><a onclick='javascript: viewTrack(" + msg["Tracks"][i].ID + ");'><img style='margin-left: 12px; height: 20px;' src='/images/eye.png' alt='show track'></a></td>" +
                        "</tr>";
            };
            
            $("#tbTrackDataBody").html(strHTML);
        },
        error: function (msg) {

            alert("Unable to load tracks...");
            console.log(JSON.stringify(msg));
        }
    });
};



function deleteTrack(trackID) {
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"chatcmd": "deleteTrack", ' +
              ' "userID": ' + $("#hfUserID").val() + ', ' +
              ' "TrackID": ' + trackID + ', ' +
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
           
            //reload dialog
            showTracks();
        },
        error: function (msg) {

            alert("Unable to delete track...");
            console.log(JSON.stringify(msg));
        }
    });
};



function waypointComparer( a, b ) {
  if ( Date.parse(a.Time) < Date.parse(b.Time )) {
    return -1;
  }
  if ( a.ChatRcpt > b.ChatRcpt ){
    return 1;
  }
  return 0;
}


function viewTrack(trackID) {
    if(g_pMap !== null) {
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"chatcmd": "getTrackData", ' +
                  ' "userID": ' + $("#hfUserID").val() + ', ' +
                  ' "TrackID": ' + trackID + ', ' +
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
                var aPolyLine = [];
                var aLine = [];

                console.log(JSON.stringify(msg));


                msg["Waypoints"].sort(waypointComparer);

                for(var i = 0; i < msg["Waypoints"].length; ++i) {
                    aPolyLine.push([msg["Waypoints"][i].Lat, msg["Waypoints"][i].Lon]);
                }
                
                aLine.push(aPolyLine);
                

                L.polyline(aLine[0]).addTo(g_pMap);

                L.polylineDecorator(aLine, {
                    patterns: [{
                        offset: 25,
                        repeat: 50,
                        symbol: L.Symbol.arrowHead({
                            pixelSize: 15,
                            pathOptions: {
                                fillOpacity: 1,
                                weight: 0
                            }
                        })
                    }]
                }).addTo(g_pMap);
            },
            error: function (msg) {

                alert("Unable to load tracks...");
                console.log(JSON.stringify(msg));
            }
        });
    };
};



function exportTrack(trackID, strName) {
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        data: '{"chatcmd": "getTrackData", ' +
              ' "userID": ' + $("#hfUserID").val() + ', ' +
              ' "TrackID": ' + trackID + ', ' +
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
            var strGPX = "";
            
            console.log(JSON.stringify(msg));
            
            msg["Waypoints"].sort(waypointComparer);
            
            //create gpx header
            strGPX  = '<?xml version="1.0"?>' +
                      '<gpx version="1.1" creator="LoRa-Link" ' +
                      'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" ' +
                      'xmlns:ogr="https://github.com/trlink/LoRaLink" ' +
                      'xmlns="http://www.topografix.com/GPX/1/1" ' +
                      'xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd"> ' + 
                      '<trk><name>' + strName + '</name><trkseg>';
            
            for(var i = 0; i < msg["Waypoints"].length; ++i) {
                strGPX += '<trkpt lat="' + msg["Waypoints"][i].Lat + '" lon="' + msg["Waypoints"][i].Lon + '">'
                strGPX += '<ele>' + msg["Waypoints"][i].Alt + '</ele>';
                strGPX += '<time>' + msg["Waypoints"][i].Time + '</time>';
                strGPX += '</trkpt>';
            };
            
            strGPX += "</trkseg></trk></gpx>";
            
            
            const file = new File([strGPX], 'track.gpx', {
                type: 'text/plain'
            });
            
            downloadFileContent(file);
        },
        error: function (msg) {

            alert("Unable to load tracks...");
            console.log(JSON.stringify(msg));
        }
    });
};



function downloadFileContent(file) {
    //variables
    ///////////
    const link = document.createElement('a');
    const url = URL.createObjectURL(file);

    link.href = url;
    link.download = file.name;
    document.body.appendChild(link);
    link.click();

    document.body.removeChild(link);
    window.URL.revokeObjectURL(url);
};