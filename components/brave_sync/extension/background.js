'use strict';

console.log("in chrome.runtime.onInstalled 001");

chrome.runtime.onStartup.addListener(function() {
  console.log("in chrome.runtime.onStartup");
  var arg1 = 142;
  chrome.braveSync.backgroundPageToBrowser("message", arg1, 143, 144, 145, function(callback_arg1) {
    console.log("in chrome.runtime.onStartup 002-4 backgroundPageToBrowser callback callback_arg1=", callback_arg1);
  });
  console.log("in chrome.runtime.onStartup 002-5");

  chrome.braveSync.onBrowserToBackgroundPage.addListener(function(arg1) {
    console.log("in chrome.braveSync.onBrowserToBackgroundPage browser-native initiated call", arg1);
  });

  chrome.braveSync.onGotInitData.addListener(function(seed, device_id, config) {
    console.log("in chrome.braveSync.onGotInitData ", JSON.stringify(arguments));
    console.log("in chrome.braveSync.onGotInitData seed=", JSON.stringify(seed));
    console.log("in chrome.braveSync.onGotInitData device_id=", JSON.stringify(device_id));
    console.log("in chrome.braveSync.onGotInitData config=", JSON.stringify(config));
    if ((seed instanceof Array && seed.length == 0) || (seed instanceof Uint8Array && seed.length == 0)) {
      seed = null;
    }
    callbackList["got-init-data"](null, seed, device_id, config);
  });

  chrome.braveSync.onFetchSyncRecords.addListener(function(category_names, start_at, max_records) {
    console.log("in chrome.braveSync.onFetchSyncRecords ", arguments);
    console.log("in chrome.braveSync.onFetchSyncRecords category_names=", category_names);
    console.log("in chrome.braveSync.onFetchSyncRecords start_at=", start_at);
    console.log("in chrome.braveSync.onFetchSyncRecords max_records=", max_records);
    callbackList["fetch-sync-records"](null, category_names, start_at, max_records);
  });

  chrome.braveSync.onResolveSyncRecords.addListener(function(category_name, recordsAndExistingObjects) {
    console.log("in chrome.braveSync.onResolveSyncRecords ", arguments);
    console.log("in chrome.braveSync.onResolveSyncRecords category_name=", category_name);
    console.log("in chrome.braveSync.onResolveSyncRecords recordsAndExistingObjects=", recordsAndExistingObjects);

    var recordsAndExistingObjectsArrArr = [];
    for(var i = 0; i < recordsAndExistingObjects.length; ++i) {
      var cur_rec = recordsAndExistingObjects[i];
      if ('localRecord' in cur_rec) {
        //console.log('have localRecord');
        fixupSyncRecordBrowserToExt(cur_rec.serverRecord);
        fixupSyncRecordBrowserToExt(cur_rec.localRecord);
        recordsAndExistingObjectsArrArr.push([cur_rec.serverRecord, cur_rec.localRecord]);
      } else {
        fixupSyncRecordBrowserToExt(cur_rec.serverRecord);
        recordsAndExistingObjectsArrArr.push([cur_rec.serverRecord, null]);
      }
    }

    console.log("in chrome.braveSync.onSendSyncRecords JSON.stringify(recordsAndExistingObjectsArrArr)=", JSON.stringify(recordsAndExistingObjectsArrArr) );

    callbackList["resolve-sync-records"](null, category_name, recordsAndExistingObjectsArrArr);
  });

  chrome.braveSync.onSendSyncRecords.addListener(function(category_name, records) {
    console.log("in chrome.braveSync.onSendSyncRecords ", arguments);
    console.log("in chrome.braveSync.onSendSyncRecords category_name=", category_name);
    console.log("in chrome.braveSync.onSendSyncRecords records=", records);

    console.log("in chrome.braveSync.onSendSyncRecords JSON.stringify(r)=", JSON.stringify(records) );

    // Fixup ids
    for (var i = 0; i < records.length; ++i) {
      fixupSyncRecordBrowserToExt(records[i]);
    }
    console.log("in chrome.braveSync.onSendSyncRecords JSON.stringify(r2)=", JSON.stringify(records) );
    callbackList["send-sync-records"](null, category_name, records);
  });

  chrome.braveSync.onNeedSyncWords.addListener(function(seed) {
    console.log("in chrome.braveSync.onGetSyncWords seed=", seed);
    var arr_int = seed.split(',').map(Number);
    console.log('arr_int=', arr_int);
    var buffer = new Uint8Array(arr_int);
    console.log('buffer=', buffer);
    var words = module.exports.passphrase.fromBytesOrHex(buffer, /*useNiceware*/true);
    console.log('words=', words);
    chrome.braveSync.syncWordsPrepared(words);
  });

  chrome.braveSync.onNeedBytesFromSyncWords.addListener(function(words) {
    console.log("in chrome.braveSync.onNeedBytesFromSyncWords words=", JSON.stringify(words));
    try {
      var bytes = module.exports.passphrase.toBytes32(words);
      console.log("in chrome.braveSync.onNeedBytesFromSyncWords bytes=", JSON.stringify(bytes));
      chrome.braveSync.bytesFromSyncWordsPrepared(bytes, '');
    } catch(err) {
      chrome.braveSync.bytesFromSyncWordsPrepared(new Uint8Array([]), err.message);
    }
  });

  chrome.braveSync.onLoadClient.addListener(function() {
    console.log("in chrome.braveSync.onLoadClient");
    LoadJsLibScript();
  });

});

chrome.runtime.onInstalled.addListener(function() {
  console.log("in chrome.runtime.onInstalled");
});

console.log("in chrome.runtime.onInstalled 003");
var theVar1 = 42;

/*
CAN DO
EXT=>BROWSER:
  chrome.braveSync.backgroundPageToBrowser

BROWSER=>EXT:
  chrome.braveSync.onBrowserToBackgroundPage.addListener(function(...))


*/

//-------------------------------------------------------------

function fixupBookmarkParentFolderObjectId(category_name, records) {
  // records[0].bookmark.parentFolderObjectId can be either Uint8Array or Array[]
  // Uint8Array is expanded to "binary",
  // Array[] is expanded to "array" of "integer" in schema
  // But schema does not support "oneOf", so forcing all to Uint8Array
  if (category_name == "BOOKMARKS") {
    for(var i = 0; i < records.length; ++i) {
      fixupSyncRecordExtToBrowser(records[i]);
    }
  }
}

function IntArrayFromString(str) {
  return str.split(",").map(Number).filter((x) => !Number.isNaN(x));
}

function fixupSyncRecordBrowserToExt(sync_record) {
  fixupSyncRecordExtToBrowser(sync_record);
  // Maybe remove syncTimeStamp
}

function fixupSyncRecordExtToBrowser(sync_record) {
  if ('objectId' in sync_record) {
    if (Array.isArray(sync_record.objectId)) {
      sync_record.objectId = new Uint8Array(sync_record.objectId);
    } else if (sync_record.objectId.length == 0) {
      sync_record.objectId = new Uint8Array();
    }
  } else {
    sync_record.objectId = new Uint8Array();
  }
  if ('objectIdStr' in sync_record) {
    if (sync_record.objectIdStr) {
      sync_record.objectId = new Uint8Array(IntArrayFromString(sync_record.objectIdStr));
    }
    delete sync_record.objectIdStr;
  }

  if ('deviceId' in sync_record) {
    if (Array.isArray(sync_record.deviceId)) {
      sync_record.deviceId = new Uint8Array(sync_record.deviceId);
    } else if (sync_record.deviceId.length == 0) {
      sync_record.deviceId = new Uint8Array();
    }
  } else {
    sync_record.deviceId = new Uint8Array();
  }
  if ('deviceIdStr' in sync_record) {
    if (sync_record.deviceIdStr) {
      sync_record.deviceId = new Uint8Array(IntArrayFromString(sync_record.deviceIdStr));
    }
    delete sync_record.deviceIdStr;
  }

  if ('bookmark' in sync_record) {
    if ('parentFolderObjectId' in sync_record.bookmark) {
      if (Array.isArray(sync_record.bookmark.parentFolderObjectId)) {
        sync_record.bookmark.parentFolderObjectId = new Uint8Array(sync_record.bookmark.parentFolderObjectId);
      } else if (sync_record.bookmark.parentFolderObjectId == null || sync_record.bookmark.parentFolderObjectId.length == 0) {
        sync_record.bookmark.parentFolderObjectId = new Uint8Array();
      }
    } else {
      sync_record.bookmark.parentFolderObjectId = new Uint8Array();
    }
    if ('parentFolderObjectIdStr' in sync_record.bookmark) {
      // if (sync_record.bookmark.parentFolderObjectIdStr) {
      //   sync_record.bookmark.parentFolderObjectId = new Uint8Array(IntArrayFromString(sync_record.bookmark.parentFolderObjectIdStr));
      // }
      delete sync_record.bookmark.parentFolderObjectIdStr;
    }
  }

}

function fixupSyncRecordsArrayExtensionToBrowser(records) {
  //
  for(var i = 0; i < records.length; ++i) {
    console.log('background.js TAGAB fixupSyncRecordsArray records[i]', JSON.stringify(records[i]));
    //'parentFolderObjectId';
    //'deviceId'
    //'objectId'
    console.log('background.js TAGAB fixupSyncRecordsArray typeof records[i].deviceId=', typeof records[i].deviceId);
    console.log('background.js TAGAB fixupSyncRecordsArray typeof records[i].objectId=', typeof records[i].objectId);
    fixupSyncRecordExtToBrowser(records[i]);
    console.log('background.js TAGAB fixupSyncRecordsArray records[i](2)', JSON.stringify(records[i]));
  }
}

class InjectedObject {
  handleMessage(message, arg1, arg2, arg3, arg4) {
    console.log('background.js TAGAB InjectedObject.handleMessage', message, arg1, arg2, arg3, arg4);
    console.log('background.js TAGAB message=' + message);
    console.log('background.js TAGAB arg1=' + arg1);
    console.log('background.js TAGAB arg2=' + arg2);
    console.log('background.js TAGAB arg3=' + arg3);
    console.log('background.js TAGAB arg4=' + arg4);

    switch (message) {
      case "get-init-data":
        console.log('background.js TAGAB will call chrome.braveSync.getInitData');
        chrome.braveSync.getInitData(arg1/*syncVersion*/);
        break;
      case "sync-setup-error":
      console.log('background.js TAGAB will call chrome.braveSync.syncSetupError');
        chrome.braveSync.syncSetupError(arg1/*error*/);
        break;
      case "sync-debug":
        chrome.braveSync.syncDebug(arg1/*message*/);
        break;
      case "save-init-data":
        chrome.braveSync.saveInitData(arg1/*seed*/, arg2/*deviceId*/);
        break;
      case "sync-ready":
        chrome.braveSync.syncReady();
        break;
      case "get-existing-objects":
         console.log('background.js get-existing-objects arg1=',JSON.stringify(arg1));
         console.log('background.js get-existing-objects arg2=',JSON.stringify(arg2));
        fixupBookmarkParentFolderObjectId(arg1, arg2);
        chrome.braveSync.getExistingObjects(arg1/*category_name*/,
          arg2/*records*/, arg3 ? arg3 : 0/*lastRecordTimeStamp*/, arg4 != undefined ? arg4 : false/*isTruncated*/);
        break;
      case "resolved-sync-records":
        console.log("in resolved-sync-records JSON.stringify(arg2)=", JSON.stringify(arg2));
        fixupSyncRecordsArrayExtensionToBrowser(arg2);
        chrome.braveSync.resolvedSyncRecords(arg1/*categoryName*/, arg2/*records*/);
        break;
      default:
        console.log('background.js TAGAB will call default chrome.braveSync.backgroundPageToBrowser????');
        chrome.braveSync.backgroundPageToBrowser(
          message ? JSON.stringify(message) : "<message?>",
          arg1 ? JSON.stringify(arg1) : "<arg1?>",
          arg2 ? JSON.stringify(arg2) : "<arg2?>",
          arg3 ? JSON.stringify(arg3) : "<arg3?>",
          arg4 ? JSON.stringify(arg4) : "<arg4?>");
        break;
    }
  }
};

var injectedObject = new InjectedObject();

//Cannot have initialization inside of dedicated function, because
//chrome.ipcRenderer should be set before sync lib main(){}() call
function dynamicallyLoadScript(url) {
  var script = document.createElement("script");
  script.src = url;
  document.head.appendChild(script);
}

function LoadJsLibScript() {
  console.log('background.js TAGAB LoadJsLibScript ----');
  var sync_bundle_url = chrome.runtime.getURL('extension/brave-sync/bundles/bundle.js');
  console.log('background.js TAGAB LoadJsLibScript sync_bundle_url=',sync_bundle_url);
  dynamicallyLoadScript(sync_bundle_url);
}

console.log('background.js TAGAB doing the call----')

var callbackList = {}; /* message name to callback function */

console.log('background.js TAGAB (2) chrome.send=', chrome.send)
if (!self.chrome) {
  self.chrome = {};
}

if(!self.chrome.ipc) {
  console.log('background.js TAGAB (3) chrome.send=', chrome.send)
  var ipc = {};

  ipc.once = function (message, cb) {
    console.log('background.js TAGAB (3.5) ipc.once, message=', message);
    callbackList[message] = cb;
    // injectedObject.handleMessage(message, '0', '0', '', false); // seems I should not call it. Just save the fn here
  };

  ipc.on = ipc.once;

  ipc.send = function (message, arg1, arg2, arg3, arg4) {
    injectedObject.handleMessage(message, arg1, arg2, arg3, arg4);
  };

  self.chrome.ipc = ipc;
  console.log('background.js TAGAB (4) chrome.send=', chrome.send)
  chrome.ipcRenderer = chrome.ipc;
}

console.log('background.js TAGAB the sync call done----')
