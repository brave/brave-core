'use strict';

console.log("in chrome.runtime.onInstalled 001");

var loaded = false;
function dynamicallyLoadScript(url) {
  var script = document.createElement("script");
  script.src = url;
  document.head.appendChild(script);
}

chrome.runtime.onStartup.addListener(function() {
  console.log("in chrome.runtime.onStartup");
})

chrome.runtime.onInstalled.addListener(function() {
  console.log("in chrome.runtime.onInstalled 002");
  var arg1 = 142;
  chrome.braveSync.backgroundPageToBrowser(arg1, function(callback_arg1) {
    console.log("in chrome.runtime.onInstalled 002-4 backgroundPageToBrowser callback callback_arg1=", callback_arg1);
  });
  console.log("in chrome.runtime.onInstalled 002-5");

  chrome.braveSync.onBrowserToBackgroundPage.addListener(function(arg1) {
    console.log("in chrome.braveSync.onBrowserToBackgroundPage browser-native initiated call", arg1);
    if (!loaded) {
      var sync_bundle_url = chrome.runtime.getURL('bundle.js');
      dynamicallyLoadScript(sync_bundle_url);
      loaded = true;
    }
  });
  //

  chrome.braveSync.onBrowserToBackgroundPageRaw.addListener(function(message, arg1, arg2, arg3, arg4) {
    console.log("in chrome.braveSync.onBrowserToBackgroundPageRaw ", message, arg1, arg2, arg3, arg4);
    callbackList[message](arg1, arg2, arg3, arg4);
  });

});

console.log("in chrome.runtime.onInstalled 003");
var theVar1 = 42;

/*
CAN DO
EXT=>BROWSER:
  chrome.braveSync.backgroundPageToBrowser

BROWSER=>EXT:
  chrome.braveSync.onBrowserToBackgroundPageRaw.addListener(function(...))


*/

//-------------------------------------------------------------
class InjectedObject {
  handleMessage(message, arg1, arg2, arg3, arg4) {
    console.log('brave_sync_lib.js TAGAB InjectedObject.handleMessage', message, arg1, arg2, arg3, arg4);
    console.log('brave_sync_lib.js TAGAB message=' + message);
    console.log('brave_sync_lib.js TAGAB arg1=' + arg1);
    console.log('brave_sync_lib.js TAGAB arg2=' + arg2);
    console.log('brave_sync_lib.js TAGAB arg3=' + arg3);
    console.log('brave_sync_lib.js TAGAB arg4=' + arg4);
    /////chrome.send('HandleMessage', [message, arg1, arg2, arg3, arg4]);
    chrome.braveSync.backgroundPageToBrowser(message, arg1, arg2, arg3, arg4);
  }
}
injectedObject = new InjectedObject()

function CallJsLib(message, arg1, arg2, arg3, arg4) {
  console.log('brave_sync_lib.js TAGAB CallJsLib ----');
  // var arg2JSON = JSON.stringify(arg2);
  // console.log('brave_sync_lib.js TAGAB CallJsLib, arg2JSON=',arg2JSON);
  console.log('brave_sync_lib.js TAGAB CallJsLib ---- message=', message);
  console.log('brave_sync_lib.js TAGAB CallJsLib ---- arg1=<',arg1,'>');
  console.log('brave_sync_lib.js TAGAB CallJsLib ---- arg2=<',arg2,'>');
  console.log('brave_sync_lib.js TAGAB CallJsLib ---- arg3=<', arg3,'>');
  console.log('brave_sync_lib.js TAGAB CallJsLib ---- arg4=<', arg4,'>');

  callbackList[message](arg1, arg2, arg3, arg4);
  ;
}

function CallJsLibStr(message, arg1, arg2, arg3, arg4) {
  console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- message=', message);
  console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- arg1=<'+arg1+'>');
  console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- arg2=<', arg2+'>');
  console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- (typeof arg2)=', (typeof arg2));
  console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- arg3=<', arg3+'>');
  console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- arg4=<', arg4+'>');

  var obj_arg1 = arg1 ? JSON.parse(arg1) : null;
  var obj_arg2;
  if (message == 'send-sync-records') {
    obj_arg2 = arg2;
  } else {
    obj_arg2 = arg2 ? JSON.parse(arg2) : null;
  }
  console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- (typeof obj_arg2)=', (typeof obj_arg2));

  var obj_arg3 = arg3 ? JSON.parse(arg3) : null;
  var obj_arg4 = arg4 ? JSON.parse(arg4) : null;

  if (message == 'words_to_bytes') {
    console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- words_to_bytes obj_arg1=', obj_arg1);
    var bytes = module.exports.passphrase.toBytes32(obj_arg1);
    chrome.send('HandleMessage', ['words_to_bytes_done', bytes]);
    return;
  } else if (message == 'bytes_to_words') {
    console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- words_to_bytes bytes_to_words=', obj_arg1);
    var arr_int = obj_arg1.split(',').map(Number);
    console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- words_to_bytes arr_int=', arr_int);
    var buffer = new Uint8Array(arr_int);
    console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- words_to_bytes buffer=', buffer);
    var words = module.exports.passphrase.fromBytesOrHex(buffer);
    chrome.send('HandleMessage', ['bytes_to_words_done', words]);
    return;
  }

  if (message == 'fetch-sync-records') {
    console.log('brave_sync_lib.js TAGAB CallJsLibStr ---- fetch-sync-records, may do pause');
  }
  callbackList[message](obj_arg1, obj_arg2, obj_arg3, obj_arg4);
}

//Cannot have initialization inside of dedicated function, because
//chrome.ipcRenderer should be set before sync lib main(){}() call
function dynamicallyLoadScript(url) {
  var script = document.createElement("script");
  script.src = url;
  document.head.appendChild(script);
}

function LoadJsLibScript() {
  console.log('brave_sync_lib.js TAGAB InitJsLib ----');
  dynamicallyLoadScript('chrome://bravesynclib/bundle.js');
}

console.log('brave_sync_lib.js TAGAB doing the call----')

var callbackList = {}; /* message name to callback function */

console.log('brave_sync_lib.js TAGAB (2) chrome.send=', chrome.send)
if (!self.chrome) {
  self.chrome = {};
}

if(!self.chrome.ipc) {
  console.log('brave_sync_lib.js TAGAB (3) chrome.send=', chrome.send)
  var ipc = {};

  ipc.once = function (message, cb) {
    console.log('brave_sync_lib.js TAGAB (3.5) ipc.once, message=', message);
    callbackList[message] = cb;
    injectedObject.handleMessage(message, '0', '0', '', false);
  };

  ipc.on = ipc.once;

  ipc.send = function (message, arg1, arg2, arg3, arg4) {
    var arg2ToPass = arg2;
    if (undefined != arg2 && typeof arg2 != 'string' && 'save-init-data' != message) {
        arg2ToPass = JSON.stringify(arg2);
    }
    injectedObject.handleMessage(message, undefined != arg1 ? arg1.toString() : '', undefined != arg2ToPass ? arg2ToPass.toString() : arg2ToPass, undefined != arg3 ? arg3.toString() : '', undefined != arg4 ? arg4 : false);
  };

  self.chrome.ipc = ipc;
  console.log('brave_sync_lib.js TAGAB (4) chrome.send=', chrome.send)
  chrome.ipcRenderer = chrome.ipc;
}

console.log('brave_sync_lib.js TAGAB the sync call done----')
//-------------------------------------------------------------
