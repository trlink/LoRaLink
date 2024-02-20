//globals
/////////
var g_strServer = "/";
var g_nLoadPos = 0;
var g_pZip = null;
var g_nFilePos = 0;
var g_aFiles = [];
var g_strCurrentDir = "/";


$(document).ready(function() {    
    
    console.log("doc ready()");
    
    $("#tabConfig").hide();
    $("#imgWaiting").attr("src", "waiting.png");
    $("#dlgWait").show();
    $("#lblDesc").text("Load Web-Application data...");
    $("#loginPanel").hide();
    $("#dlgWait").hide();
    $("#loginPanel").show();
});




function parseIntDefault(value, nDefault) {
       
    if((value !== null) && (value !== undefined) && (isNaN(value) != true) && (value.length > 0)) {
        return parseInt(value);
    }
    
    return nDefault;
}


function parseFloatDefault(value, fDefault) {
       
    if((value !== null) && (value !== undefined) && (isNaN(value) != true) && (value.length > 0)) {
        return parseFloat(value);
    }
    
    return fDefault;
}



function btnLogin_Click() {
    //variables
    ///////////
    var strAdminUser = encodeURI($("#txtUsername").val());
    
    if((strAdminUser.length > 0) && (strAdminUser.length < 25)) {
        
        $("#dlgWait").show();
        $("#lblDesc").text("Validate User & Password...");
        
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            data: '{"command": "AdminLogin", ' +
                  ' "Username": "' + strAdminUser + '", ' +
                  ' "Password": "' + ($("#txtPassword").val().length > 0 ? CryptoJS.MD5($("#txtPassword").val()) : "") + '"}',
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
                    $("#txtAdminUser").val($("#txtUsername").val());
                    $("#txtAdminPassword").val($("#txtPassword").val());
                    $("#txtAdminPasswordConf").val($("#txtPassword").val());
                    
                    $("#loginPanel").hide();
                    $("#hfPwdHash").val(($("#txtPassword").val().length > 0 ? CryptoJS.MD5($("#txtPassword").val()) : ""));
                    $("#hfAdminUser").val(strAdminUser);
                    
                    $("#tabConfig").show();
                    $("#dlgWait").hide();
                }
                else {
                    $("#dlgWait").hide();
                };
            },
            error: function (msg) {
                console.log(JSON.stringify(msg));

                $("#dlgWait").hide();
            }
        });
    };
};



function btnSaveAdminUser_Click() {
    //variables
    ///////////
    var strAdminUser = encodeURI($("#txtAdminUser").val());
    
    if(($("#txtAdminPassword").val() === $("#txtAdminPasswordConf").val()) && (strAdminUser.length < 25)) {
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            crossDomain: true,
            data: '{"command": "UpdateAdminLogin", ' +
                  ' "OldUsername": "' + $("#hfAdminUser").val() + '", ' +
                  ' "OldPassword": "' + $("#hfPwdHash").val() + '", ' +
                  ' "szUser": "' + strAdminUser + '", ' +
                  ' "szPassword": "' + ($("#txtAdminPassword").val().length > 0 ? CryptoJS.MD5($("#txtAdminPassword").val()) : "") + '"}',
            contentType: 'application/json; charset=utf-8',
            dataType: 'json',
            async: false,
            success: function(msg) {
                console.log(JSON.stringify(msg));

                if(msg["response"] == "OK") {
                    
                    $("#hfPwdHash").val(($("#txtAdminPassword").val().length > 0 ? CryptoJS.MD5($("#txtAdminPassword").val()) : ""));
                    $("#hfAdminUser").val($("#txtAdminUser").val());
                    
                    alert("Saved!");
                }
                else {
                    alert("Error!");
                };
            },
            error: function (msg) {
                console.log(JSON.stringify(msg));
                alert("Error!");
            }
        });
    }
    else {
        alert("Passwords doesnt match or Username to long!");
    };
};



function getDDNSConfig() {
    
    $("#dlgWait").show();
    $("#lblDesc").text("Get Dynamic DNS config...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "GetDDNSCfg", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: false,
        success: function(msg) {
            console.log(JSON.stringify(msg));

            if(msg["response"] === "OK") {
                $("#cmbSettingsDDNSType").selectpicker('val', msg["szProvider"]);
                $("#txtDDNSUserName").val(msg["szUser"]);
                $("#txtDDNSPassword").val(msg["szPassword"]);
                $("#txtDomainName").val(msg["szDomain"]);
                $("#chkAuthenticateToken").prop('checked', msg["bAuthWithUser"]);
            }
            else {
                $("#lblDesc").text("Failed to load Dynamic DNS config...");
            };
            
            $("#dlgWait").hide();
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load Dynamic DNS config...");
            $("#dlgWait").hide();
        }
    });
};


function getFilesystemConfig() {
    //variables
    ///////////
    var dtTable = null;
    
    
    $("#dlgWait").show();
    $("#lblDesc").text("Get Filesystem config...");
            
    if($.fn.dataTable.isDataTable('#dtFilesList')) {
        $('#dtFilesList').empty();
    };
    
    
    
    $("#file").prop("disabled", true);
    $("#frmFilesCreateFolder").hide();
    
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "GetFilesystemCfg", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            console.log(JSON.stringify(msg));

            $('#cmbFileSystem').children().remove();

            if(msg["response"] == "OK") {
                if(msg["SPIFFS"] == true) {
                    $('#cmbFileSystem').append($('<option>', {
                        value: "SPIFFS",
                        text: "Internal Filesystem"
                    }));
                };
                
                if(msg["SDCARD"] == true) {
                    $('#cmbFileSystem').append($('<option>', {
                        value: "SDCARD",
                        text: "External Filesystem"
                    }));
                };
                
                $('#cmbFileSystem').selectpicker('refresh');
                $('#cmbFileSystem').selectpicker('render');

                $('#cmbFileSystem').on('change', function(e) {
                    $("#file").prop("disabled", false);
                    
                    if(this.value === "SDCARD") {
                        $("#frmFilesCreateFolder").show();
                    }
                    else {
                        $("#frmFilesCreateFolder").hide();
                    };
                    
                    loadFilesFromDev(this.value, "/");
                });
            }
            else {
                $("#lblDesc").text("Failed to load filesystem config...");
            };
            
            $("#dlgWait").hide();
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load filesystem config...");
            $("#dlgWait").hide();
        }
    });
};


function btnFilesNewFolder_Click() {
    if($("#txtFilesNewFolder").val().length > 0) {
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            crossDomain: true,
            data: '{"command": "createFolder", ' +
                  ' "NewFolder": "' + $("#txtFilesNewFolder").val() + '", ' +
                  ' "Folder": "' + g_strCurrentDir + '", ' +
                  ' "Username": "' + $("#hfAdminUser").val() + '", ' +
                  ' "Password": "' + $("#hfPwdHash").val() + '"}',
            contentType: 'application/json; charset=utf-8',
            dataType: 'json',
            async: true,
            success: function(msg) {
                $("#txtFilesNewFolder").val("");
                loadFilesFromDev($('#cmbFileSystem').val(), g_strCurrentDir);
            },
            error: function (msg) {
                console.log("Error create folder:");

                console.log(JSON.stringify(msg));
            }
        });
    };
};



function loadFilesFromDev(strFileSystem, strFolder) {
    
    //get prev folder
    if(strFolder === "..") {
        var path   = g_strCurrentDir;
        var parts  = path.split("/");
        var folder = parts.slice(0, -2).join("/");
        
        if(folder.length <= 0) {
            strFolder = "/";
        }
        else {
            strFolder = folder;
        };
        
        console.log("Change folder to: " + strFolder);
    };
    
    g_strCurrentDir = strFolder;
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "getFiles", ' +
              ' "FileSystem": "' + strFileSystem + '", ' +
              ' "Folder": "' + g_strCurrentDir + '", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            //variables
            ///////////
            var dtTable = null;
            var strFile = "";
            var bFound  = true;
            var i = 0;
            
            console.log(JSON.stringify(msg));
            
            if(strFileSystem.length > 0) {
                $("#lblFreespace").text(msg["Free"]);
                $("#lblUsedspace").text(msg["Used"]);
                $("#lblTotalspace").text(parseInt(msg["Used"]) + parseInt(msg["Free"]));

                if($.fn.dataTable.isDataTable('#dtFilesList')) {
                    dtTable = $('#dtFilesList').DataTable();
                    dtTable.destroy();
                };

                if($("#chkFileManagementConfigOnly").prop('checked') === true) {

                    $("#btnSaveConfigFiles").show();

                    while(bFound === true) {
                        bFound = false;

                        for(i = 0; i < msg["Files"].length; ++i) {
                            strFile = msg["Files"][i][1];

                            if(strFile.toLowerCase().indexOf(".html") !== -1) {
                                msg["Files"].splice(i, 1);
                                bFound = true;
                            };

                            if(strFile.toLowerCase().indexOf(".css") !== -1) {
                                msg["Files"].splice(i, 1);
                                bFound = true;
                            };

                            if(strFile.toLowerCase().indexOf(".png") !== -1) {
                                msg["Files"].splice(i, 1);
                                bFound = true;
                            };

                            if(strFile.toLowerCase().indexOf(".jpg") !== -1) {
                                msg["Files"].splice(i, 1);
                                bFound = true;
                            };

                            if((strFile.toLowerCase().indexOf(".js") !== -1) && (strFile.toLowerCase().indexOf(".jsn") === -1)) {
                                msg["Files"].splice(i, 1);
                                bFound = true;
                            };

                            if(strFile.toLowerCase().indexOf(".wav") !== -1) {
                                msg["Files"].splice(i, 1);
                                bFound = true;
                            };
                        }
                    }

                    g_aFiles = msg["Files"];
                }

                new DataTable('#dtFilesList', {
                    data: msg["Files"],
                    columns: [
                        {
                            title: 'isDir',
                            visible: false,
                            searchable: false
                        },
                        {
                            title: 'Name'
                        },
                        {
                            title: 'Size',
                            searchable: false
                        },
                        {
                            searchable: false,
                            title: ''
                        }
                    ]
                });
            }
            else {
                $("#lblFreespace").text("0");
                $("#lblUsedspace").text("0");
                $("#lblTotalspace").text("0");
            };
        },
        error: function (msg) {
            console.log("Error to load files:");
            
            console.log(JSON.stringify(msg));
        }
    });
};


function onChkFileManagementConfigOnly_Change() {
    
    if($("#chkFileManagementConfigOnly").prop('checked') === false) {
        $("#btnSaveConfigFiles").hide();
    };
        
    loadFilesFromDev($('#cmbFileSystem').val(), g_strCurrentDir);
};


function btnSaveConfigFiles_Click() {
    g_pZip      = new JSZip();
    g_nFilePos  = 0;
    var strFile = g_strCurrentDir + g_aFiles[g_nFilePos][1].substring(g_aFiles[g_nFilePos][1].indexOf(">") + 1, g_aFiles[g_nFilePos][1].indexOf("<", 1));
    
    downloadFile(strFile, onDownloadComplete);
};


function downloadFile(url, onSuccess) {
    var xhr = new XMLHttpRequest();
    
    console.log("Start download: " + url);
    
    xhr.open('GET', url, true);
    xhr.responseType = "blob";
    
    xhr.onreadystatechange = function () {
        if (xhr.readyState === 4) {
            if (onSuccess) onSuccess(xhr.response);
        }
    };
    
    xhr.send();
};


function onDownloadComplete(blobData) {
    if (g_nFilePos < g_aFiles.length) {
        blobToBase64(blobData, function(binaryData) {
            // add downloaded file to zip:
            var strFile     = g_strCurrentDir + g_aFiles[g_nFilePos][1].substring(g_aFiles[g_nFilePos][1].indexOf(">") + 1, g_aFiles[g_nFilePos][1].indexOf("<", 1));
            var fileName    = strFile.substring(strFile.lastIndexOf('/') + 1);

            g_pZip.file(fileName, binaryData, {base64: true});

            if(g_nFilePos < g_aFiles.length -1){
                g_nFilePos++;
                
                strFile     = g_strCurrentDir + g_aFiles[g_nFilePos][1].substring(g_aFiles[g_nFilePos][1].indexOf(">") + 1, g_aFiles[g_nFilePos][1].indexOf("<", 1));
                
                downloadFile(strFile, onDownloadComplete);
            }
            else {
                // all files have been downloaded, create the zip
                g_pZip.generateAsync({type:"blob"})
                    .then(function (blob) {
                        saveAs(blob, "LoRaLinkCfg.zip");
                    });
            }
        });
    };
};


function blobToBase64(blob, callback) {
    var reader = new FileReader();
    reader.onload = function() {
        var dataUrl = reader.result;
        var base64 = dataUrl.split(',')[1];
        callback(base64);
    };
    reader.readAsDataURL(blob);
}


function openDirectory(strDir) {
    loadFilesFromDev($('#cmbFileSystem').val(), strDir);
};


function deleteFile(strFile) {
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "deleteFile", ' +
              ' "File": "' + strFile + '", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            console.log(JSON.stringify(msg));

            loadFilesFromDev($('#cmbFileSystem').val(), "/");
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));
        }
    });
};


function uploadFile(e) {
    //variables
    ///////////
    var form      = document.getElementById("frmFileUpload");
    var file      = e.target.files[0].name;
    var formdata  = new FormData(form);
    var ajax      = new XMLHttpRequest();


    $("#lblUploadStatus").text("Uploading " + g_strCurrentDir + file + "...");

    formdata.append("file", file);
    
    ajax.addEventListener("load", completeHandler, false); 
    ajax.addEventListener("error", errorHandler, false);
    ajax.addEventListener("load", completeHandler, false); 
    ajax.open("POST", "/upload?folder=" + encodeURI(g_strCurrentDir));
    ajax.send(formdata);
};



function errorHandler(event) {
    $("#lblUploadStatus").text("Upload failed");
    $("#file").val("");
};


function completeHandler(event) {
    
    $("#lblUploadStatus").text("Upload Complete");
    
    loadFilesFromDev($('#cmbFileSystem').val(), g_strCurrentDir);
    
    $("#file").val("");
};





function getModemConfig() {
    
    $("#dlgWait").show();
    $("#lblDesc").text("Get LoRa Modem config...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "GetModemCfg", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: false,
        success: function(msg) {
            console.log(JSON.stringify(msg));

            if(msg["response"] == "OK") {
                $("#chkDisableLoraModem").prop('checked', msg["DisableLoRa"]);
                $("#txtFrequency").val(msg["Freq"]);
                $("#cmbBandwith").selectpicker('val', msg["BW"]);
                $("#cmbCodingRate").selectpicker('val', msg["CR"]);
                $("#cmbSpreadingFactor").selectpicker('val', msg["SF"]);
                $("#txtSyncWord").val(msg["SW"]);
                $("#cmbTransmitPwr").val(msg["Power"]);
                $("#txtPreamble").val(msg["Preamble"]);
                $("#txtIntMsgInt").val(msg["MsgTxInt"]);
                $("#txtModemReceiveTimeout").val(msg["nTransmissionReceiveTimeout"]);
                $("#txtModemTxDelay").val(msg["lModemTxDelay"]);
                $("#txtModemRxDelay").val(msg["lModemRxDelay"]);
            }
            else {
                $("#lblDesc").text("Failed to load Modem config...");
            };
            
            //load next config
            $("#dlgWait").hide();
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load Modem config...");
            $("#dlgWait").hide();
        }
    });
};



function getWiFiApConfig() {
    
    $("#dlgWait").show();
    $("#lblDesc").text("Get WiFi Accesspoint config...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "GetWiFiAP", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: false,
        success: function(msg) {
            console.log(JSON.stringify(msg));

            if(msg["response"] == "OK") {
                $("#txtWiFiAp_SSID").val(msg["szWLANSSID"]);
                $("#txtWiFiAp_Password").val(msg["szWLANPWD"]);
                $("#txtWiFiAp_IP").val(msg["szDevIP"]);
                $("#chkWiFiAP_HideNetwork").prop('checked', msg["bHideNetwork"]);
                $("#txtWiFiAp_Channel").val(msg["nChannel"]);
                $("#cmbWiFiAP_Power").selectpicker('val', msg["power"]);
                $("#chkWiFiAP_Disabled").prop('checked', msg["bDisabled"]);
            }
            else {
                $("#lblDesc").text("Failed to load WiFiAP config...");
            };
            
            //load next config
            $("#dlgWait").hide();
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load WiFiAP config...");
            $("#dlgWait").hide();
        }
    });
};



function getWiFiConfig() {
    
    $("#dlgWait").show();
    $("#lblDesc").text("Get WiFi config...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "GetWiFi", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "scanWiFi": 1, ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: false,
        success: function(msg) {
            console.log(JSON.stringify(msg));
            
            let i = 0;
            var inserted = [];
            
            if(msg["response"] == "OK") {
                
                $('#cmbWiFiNetwork').children().remove();
                
                $('#cmbWiFiNetwork').append($('<option>', {
                    value: "",
                    text:  ""
                }));
                
                for (i = 0; i < msg["networks"].length; i++) {
                    
                    if(inserted.includes(msg["networks"][i]) == false) {
                    
                        console.log(msg["networks"][i]);

                        $('#cmbWiFiNetwork').append($('<option>', {
                            value: msg["networks"][i],
                            text: msg["networks"][i]
                        }));

                        inserted.push(msg["networks"][i]);
                    };
                };
                
                if(msg["szWLANSSID"].length > 0) {
                    //append configurred net to list, if not avail
                    if(msg["networks"].includes(msg["szWLANSSID"]) == false) {
                        $('#cmbWiFiNetwork').append($('<option>', {
                            value: msg["szWLANSSID"],
                            text:  msg["szWLANSSID"]
                        }));
                    };
                    
                    $('#cmbWiFiNetwork').selectpicker('refresh');
                    $('#cmbWiFiNetwork').selectpicker('render');
                
                    $("#cmbWiFiNetwork").selectpicker('val', msg["szWLANSSID"]);
                }
                else {
                    $('#cmbWiFiNetwork').selectpicker('refresh');
                    $('#cmbWiFiNetwork').selectpicker('render');
                
                    $("#cmbWiFiNetwork").selectpicker('val', "");
                };
                
                
                $("#txtWiFi_Password").val(msg["szWLANPWD"]);
                
                $("#dlgWait").hide();
            }
            else {
                $("#lblDesc").text("Failed to load WiFi config...");
            };
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load WiFi config...");
            $("#dlgWait").hide();
        }
    });
};




function getDeviceConfig() {
    
    $("#dlgWait").show();
    $("#lblDesc").text("Get Device config...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "GetDeviceCfg", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: false,
        success: function(msg) {
            console.log(JSON.stringify(msg));
            
            if(msg["response"] == "OK") {
                
                $("#cmbSettingsDeviceType").selectpicker('val', msg["nDeviceType"]);
                $("#chkSettingsTimeUPD").prop('checked', msg["bUpdateTimeNTP"]);
                $("#txtSettingsDeviceName").val(decodeURI(msg["szDevName"]));
                $("#txtSettingsDeviceOwner").val(decodeURI(msg["szDevOwner"]));
                $("#txtSettingsDeviceID").val(msg["dwDeviceID"]);
                $("#txtSettingsDeviceLat").val(msg["fLocN"]);
                $("#txtSettingsDeviceLon").val(msg["fLocE"]);
                $("#txtSettingsDeviceBlocked").val(msg["szBlockedNodes"]);
                $("#txtSettingsDeviceShoutOut").val(msg["nMaxShoutOutEntries"]);
                $("#txtSettingsMaxUser").val(msg["nMaxUser"]);
                
                $("#dlgWait").hide();
            }
            else {
                $("#lblDesc").text("Failed to load Device config...");
                $("#dlgWait").hide();
            };
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load Device config...");
            $("#dlgWait").hide();
        }
    });
};


function getLinkConfig() {
    
    $("#lblDesc").text("Get Link config...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "GetLinkCfg", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: false,
        success: function(msg) {
            console.log(JSON.stringify(msg));
            
            if(msg["response"] == "OK") {
                
                $("#chkLink_ServerEnabled").prop('checked', msg["bServerEnabled"]);
                $("#chkLink_ClientEnabled").prop('checked', msg["bClientEnabled"]);
                $("#txtLink_ServerPort").val(msg["wServerPort"]);
                $("#txtLink_ClientPort").val(msg["wClientPort"]);
                $("#txtLink_ClientName").val(msg["szClient"]);
                
                //show config pane
                $("#dlgWait").hide();
                $("#tabConfig").show();
            }
            else {
                $("#lblDesc").text("Failed to load Device config...");
            };
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load Device config...");
        }
    });
};



function getUserList() {
    
    $("#dlgWait").show();
    $("#lblDesc").text("Get Userlist...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "getUserList", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            //variables
            ///////////
            var nID = 0;
            var dtTable = null;
            
            console.log(JSON.stringify(msg));
            $("#hfUserManagementUser").val(0);
            
            
            if($.fn.dataTable.isDataTable('#dtUserList')) {
                dtTable = $('#dtUserList').DataTable();
                dtTable.destroy();
            };
            
            
            new DataTable('#dtUserList', {
                data: msg["User"],
                columns: [
                    {
                        title: 'ID',
                        visible: false,
                        searchable: false
                    },
                    {
                        title: 'User Name'
                    },
                    {
                        title: 'Last Login'
                    },
                    {
                        title: 'Last Heard'
                    },
                    {
                        title: 'User Created'
                    },
                    {
                        title: 'User Mail'
                    },
                    {
                        title: 'Forward Msg'
                    },
                    {
                        visible: false,
                        searchable: false,
                        title: 'Show Date'
                    },
                    {
                        title: 'User Blocked'
                    }
                ]
            });
                    

            $("#dtUserList tbody").on('click', 'tr', function () {
                //variables
                ///////////
                var dtTable = $('#dtUserList').DataTable();
                var data = dtTable.row(this).data(); 
               
               if(data[8] === "0") {
                   $("#btnUserManagementUnblock").hide();
                   $("#btnUserManagementBlock").show();
               }
               else {
                   $("#btnUserManagementUnblock").show();
                   $("#btnUserManagementBlock").hide();
               };
                   
               
               $("#dlgUserManagementChangeUserLongTitle").text(data[1]);
               $("#hfUserManagementUser").val(data[0]);
               $("#dlgUserManagementChangeUser").modal('show');
            });
            
            $("#dlgWait").hide();
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load User config...");
            $("#dlgWait").hide();
        }
    });
};


function getNodeList() {
    
    $("#dlgWait").show();
    $("#lblDesc").text("Get Nodes...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "getNodeList", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            //variables
            ///////////
            var nID = 0;
            var dtTable = null;
            
            console.log(JSON.stringify(msg));
            
            if($.fn.dataTable.isDataTable('#dtNodeList')) {
                dtTable = $('#dtNodeList').DataTable();
                dtTable.destroy();
            };
            
            
            new DataTable('#dtNodeList', {
                data: msg["Nodes"],
                columns: [
                    {
                        title: 'ID',
                        visible: false,
                        searchable: false
                    },
                    {
                        title: 'Node ID'
                    },
                    {
                        title: 'Node Name'
                    },
                    {
                        title: 'Last Heard'
                    },
                    {
                        title: 'Latitude'
                    },
                    {
                        title: 'Longitude'
                    },
                    {
                        title: 'RSSI'
                    },
                    {
                        title: 'SNR'
                    }
                ]
            });
            
            $("#dlgWait").hide();
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load Nodes...");
            $("#dlgWait").hide();
        }
    });
};



function getRouting() {
    
    $("#lblDesc").text("Get routing...");
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "getRoutes", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            //variables
            ///////////
            var nID = 0;
            var dtTable = null;
            
            console.log(JSON.stringify(msg));
            
            if($.fn.dataTable.isDataTable('#dtRouteList')) {
                dtTable = $('#dtRouteList').DataTable();
                dtTable.destroy();
            };
            
            
            new DataTable('#dtRouteList', {
                data: msg["Routes"],
                columns: [
                    {
                        title: 'Node ID'
                    },
                    {
                        title: 'Via Node'
                    },
                    {
                        title: 'Type'
                    },
                    {
                        title: 'Hop Count'
                    },
                    {
                        title: 'Dev Type'
                    }
                ]
            });
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            $("#lblDesc").text("Failed to load routes...");
        }
    });
};




function btnReset_Click() {
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "deviceReset", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));
        }
    });
}

function btnUserManagementDelete_click() {
    if($("#hfUserManagementUser").val() > 0) {
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            crossDomain: true,
            data: '{"command": "deleteUser", ' +
                  ' "Username": "' + $("#hfAdminUser").val() + '", ' +
                  ' "ID": "' + $("#hfUserManagementUser").val() + '", ' +
                  ' "Password": "' + $("#hfPwdHash").val() + '"}',
            contentType: 'application/json; charset=utf-8',
            dataType: 'json',
            async: true,
            success: function(msg) {
                console.log(JSON.stringify(msg));

                getUserList(false);
            },
            error: function (msg) {
                console.log(JSON.stringify(msg));

                $("#lblDesc").text("Failed to delete user...");
            }
        });
    };
};


function btnUserManagementReset_click() {
    if($("#hfUserManagementUser").val() > 0) {
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            crossDomain: true,
            data: '{"command": "resetUserPwd", ' +
                  ' "Username": "' + $("#hfAdminUser").val() + '", ' +
                  ' "ID": "' + $("#hfUserManagementUser").val() + '", ' +
                  ' "newPassword": "' + CryptoJS.MD5("loralink") + '", ' +
                  ' "Password": "' + $("#hfPwdHash").val() + '"}',
            contentType: 'application/json; charset=utf-8',
            dataType: 'json',
            async: true,
            success: function(msg) {
                console.log(JSON.stringify(msg));
                
                alert("Password set to 'loralink', the user should now change his password!");

                getUserList(false);
            },
            error: function (msg) {
                console.log(JSON.stringify(msg));

                $("#lblDesc").text("Failed to change user...");
            }
        });
    };
}


function btnUserManagementBlock_click(nBlock) {
    if($("#hfUserManagementUser").val() > 0) {
        $.ajax({
            url: g_strServer + 'api/api.json',
            type: 'POST',
            crossDomain: true,
            data: '{"command": "blockUser", ' +
                  ' "Username": "' + $("#hfAdminUser").val() + '", ' +
                  ' "ID": "' + $("#hfUserManagementUser").val() + '", ' +
                  ' "blocked": ' + nBlock + ', ' +
                  ' "Password": "' + $("#hfPwdHash").val() + '"}',
            contentType: 'application/json; charset=utf-8',
            dataType: 'json',
            async: true,
            success: function(msg) {
                console.log(JSON.stringify(msg));

                $("#dlgUserManagementChangeUser").modal('hide');
                
                getUserList();
            },
            error: function (msg) {
                console.log(JSON.stringify(msg));

                alert("Failed to block/unblock user...");
            }
        });
    }
    else {
        console.log("User ID not set!");
    }
};



function btnSaveModem_Click() {
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "SetModemCfg", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "Freq": ' + $("#txtFrequency").val() + ', ' +
              ' "SF": ' + $("#cmbSpreadingFactor").val() + ', ' +
              ' "BW": ' + $("#cmbBandwith").val() + ', ' +
              ' "DisableLoRa": ' + $("#chkDisableLoraModem").prop('checked')  + ', ' +
              ' "Power": ' + $("#cmbTransmitPwr").val() + ', ' +
              ' "Preamble": ' + $("#txtPreamble").val() + ', ' +
              ' "SW": ' + $("#txtSyncWord").val() + ', ' +
              ' "CR": ' + $("#cmbCodingRate").val() + ', ' +
              ' "lModemTxDelay": ' + $("#txtModemTxDelay").val() + ', ' +
              ' "lModemRxDelay": ' + $("#txtModemRxDelay").val() + ', ' +
              ' "MsgTxInt": ' + $("#txtIntMsgInt").val() + ', ' +
              ' "nTransmissionReceiveTimeout": ' + $("#txtModemReceiveTimeout").val() + ', ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            console.log(JSON.stringify(msg));
            
            if(msg["response"] == "OK") {
                alert("Saved!");
            }
            else {
                alert("Error!");
            };
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            alert("Error!");
        }
    });
}


function isNameValid(name) {
  /* 
    Usernames can only have: 
    - Lowercase Letters (a-z) 
    - Numbers (0-9)
    - Dots (.)
    - Underscores (_)
  */
  const res = /^[a-zA-Z0-9_\.\-]+$/.exec(name);
  const valid = !!res;
  return valid;
};


function btnSaveDevice_Click() {
    //variables
    ///////////
    var strDevName  = $("#txtSettingsDeviceName").val();
    var strDevOwner = encodeURI($("#txtSettingsDeviceOwner").val());
    
    if((strDevName.length < 25) && (strDevOwner.length < 200))
    {
        if(isNameValid(strDevName) === true) {
            $.ajax({
                url: g_strServer + 'api/api.json',
                type: 'POST',
                crossDomain: true,
                data: '{"command": "SetDeviceCfg", ' +
                      ' "Username": "' + $("#hfAdminUser").val() + '", ' +
                      ' "szBlockedNodes": "' + $("#txtSettingsDeviceBlocked").val() + '", ' +
                      ' "dwDeviceID": ' + $("#txtSettingsDeviceID").val() + ', ' +
                      ' "szDevName": "' + strDevName + '", ' +
                      ' "szDevOwner": "' + strDevOwner + '", ' +
                      ' "nDeviceType": ' + $("#cmbSettingsDeviceType").val() + ', ' +
                      ' "nMaxUser": ' + $("#txtSettingsMaxUser").val() + ', ' +
                      ' "nMaxShoutOutEntries": ' + $("#txtSettingsDeviceShoutOut").val() + ', ' +
                      ' "fLocN": ' + parseFloatDefault($("#txtSettingsDeviceLat").val(), 0.0) + ', ' +
                      ' "fLocE": ' + parseFloatDefault($("#txtSettingsDeviceLon").val(), 0.0) + ', ' +
                      ' "bUpdateTimeNTP": ' + $("#chkSettingsTimeUPD").prop('checked')  + ', ' +
                      ' "Password": "' + $("#hfPwdHash").val() + '"}',
                contentType: 'application/json; charset=utf-8',
                dataType: 'json',
                async: true,
                success: function(msg) {
                    console.log(JSON.stringify(msg));

                    if(msg["response"] == "OK") {
                        alert("Saved!");
                    }
                    else {
                        alert("Error!");
                    };
                },
                error: function (msg) {
                    console.log(JSON.stringify(msg));

                    alert("Error!");
                }
            });
        }
        else {
            alert("Device Name can only contain a-z, A-Z, digits, dots, minus or underscore!");
        };
    }
    else {
        alert("Device Name or Owner to long!");
    };
}


function btnSaveWiFiAP_Click() {
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "SetWiFiAP", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "szWLANSSID": "' + $("#txtWiFiAp_SSID").val() + '", ' +
              ' "szWLANPWD": "' + $("#txtWiFiAp_Password").val() + '", ' +
              ' "szDevIP": "' + $("#txtWiFiAp_IP").val() + '", ' +
              ' "bHideNetwork": ' + $("#chkWiFiAP_HideNetwork").prop('checked')  + ', ' +
              ' "bDisabled": ' + $("#chkWiFiAP_Disabled").prop('checked')  + ', ' +
              ' "power": ' + parseIntDefault($("#cmbWiFiAP_Power").val(), 78) + ', ' +
              ' "nChannel": ' + parseIntDefault($("#txtWiFiAp_Channel").val(), 1) + ', ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            console.log(JSON.stringify(msg));

            if(msg["response"] == "OK") {
                alert("Saved!");
                
            }
            else {
                alert("Error!");
            };
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            alert("Error!");
        }
    });
}


function btnSaveWiFi_Click() {
    
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "SetWiFi", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "szWLANSSID": "' + $("#cmbWiFiNetwork option:selected").text() + '", ' +
              ' "szWLANPWD": "' + $("#txtWiFi_Password").val() + '", ' +
              ' "szDevIP": "' + '", ' +
              ' "bHideNetwork": false' + ', ' +
              ' "nChannel": 1' + ', ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            console.log(JSON.stringify(msg));
                        
            if(msg["response"] == "OK") {
                alert("Saved!");
            }
            else {
                alert("Error!");
            };
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            alert("Error!");
        }
    });
        
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "SetDDNSCfg", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "szProvider": "' + $("#cmbSettingsDDNSType").val() + '", ' +
              ' "szUser": "' + $("#txtDDNSUserName").val() + '", ' +
              ' "szPassword": "' + $("#txtDDNSPassword").val() +  '", ' +
              ' "bAuthWithUser": ' + $("#chkAuthenticateToken").prop('checked') + ', ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            console.log(JSON.stringify(msg));
                        
            if(msg["response"] == "OK") {
                alert("Saved DDNS!");
            }
            else {
                alert("Error!");
            };
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            alert("Error!");
        }
    });
};




function btnSaveLink_Click() {
    $.ajax({
        url: g_strServer + 'api/api.json',
        type: 'POST',
        crossDomain: true,
        data: '{"command": "SetLinkCfg", ' +
              ' "Username": "' + $("#hfAdminUser").val() + '", ' +
              ' "wServerPort": ' + $("#txtLink_ServerPort").val() + ', ' +
              ' "bServerEnabled": ' + $("#chkLink_ServerEnabled").prop('checked') + ', ' +
              ' "bClientEnabled": ' + $("#chkLink_ClientEnabled").prop('checked') + ', ' +
              ' "szClient": "' + $("#txtLink_ClientName").val() + '", ' +
              ' "wClientPort": ' + $("#txtLink_ClientPort").val() + ', ' +
              ' "Password": "' + $("#hfPwdHash").val() + '"}',
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(msg) {
            console.log(JSON.stringify(msg));
                        
            if(msg["response"] == "OK") {
                alert("Saved!");
            }
            else {
                alert("Error!");
            };
        },
        error: function (msg) {
            console.log(JSON.stringify(msg));

            alert("Error!");
        }
    });
};