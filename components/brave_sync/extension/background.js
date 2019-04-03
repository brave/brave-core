'use strict';

chrome.braveSync.onGotInitData.addListener(function(seed, device_id, config, sync_words) {
  if ((seed instanceof Array && seed.length == 0) || (seed instanceof Uint8Array && seed.length == 0)) {
    seed = null;
  }
  if (sync_words) {
    try {
      seed = module.exports.passphrase.toBytes32(sync_words)
    } catch(err) {
      console.log(`"onGotInitData" sync_words=${JSON.stringify(sync_words)} err.message=${err.message}`);
      chrome.braveSync.syncSetupError('ERR_SYNC_WRONG_WORDS')
      return;
    }
  }
  console.log(`"got-init-data" seed=${JSON.stringify(seed)} device_id=${JSON.stringify(device_id)} config=${JSON.stringify(config)}`);
  callbackList["got-init-data"](null, seed, device_id, config);
});

chrome.braveSync.onFetchSyncRecords.addListener(function(category_names, start_at, max_records) {
  console.log(`"fetch-sync-records" category_names=${JSON.stringify(category_names)} start_at=${JSON.stringify(start_at)} max_records=${JSON.stringify(max_records)}`);
  callbackList["fetch-sync-records"](null, category_names, start_at, max_records);
});

chrome.braveSync.onFetchSyncDevices.addListener(function() {
  console.log("fetch-sync-devices");
  callbackList["fetch-sync-devices"](null);
});

chrome.braveSync.onResolveSyncRecords.addListener(function(category_name, recordsAndExistingObjects) {
  var recordsAndExistingObjectsArrArr = [];
  for(var i = 0; i < recordsAndExistingObjects.length; ++i) {
    var cur_rec = recordsAndExistingObjects[i];
    if ('localRecord' in cur_rec) {
      fixupSyncRecordBrowserToExt(cur_rec.serverRecord);
      fixupSyncRecordBrowserToExt(cur_rec.localRecord);
      getOrder(cur_rec.localRecord);
      removeLocalMeta(cur_rec.serverRecord);
      removeLocalMeta(cur_rec.localRecord);
      recordsAndExistingObjectsArrArr.push([cur_rec.serverRecord, cur_rec.localRecord]);
    } else {
      fixupSyncRecordBrowserToExt(cur_rec.serverRecord);
      removeLocalMeta(cur_rec.serverRecord);
      recordsAndExistingObjectsArrArr.push([cur_rec.serverRecord, null]);
    }
  }
  console.log(`"resolve-sync-records" category_name=${JSON.stringify(category_name)} recordsAndExistingObjects=${JSON.stringify(recordsAndExistingObjectsArrArr)}`);
  callbackList["resolve-sync-records"](null, category_name, recordsAndExistingObjectsArrArr);
});

chrome.braveSync.onSendSyncRecords.addListener(function(category_name, records) {
  // Fixup ids
  for (var i = 0; i < records.length; ++i) {
    fixupSyncRecordBrowserToExt(records[i]);
    getOrder(records[i]);
    removeLocalMeta(records[i]);
  }
  console.log(`"send-sync-records" category_name=${JSON.stringify(category_name)} records=${JSON.stringify(records)}`);
  callbackList["send-sync-records"](null, category_name, records);
  if (category_name == 'BOOKMARKS') {
    fixupSyncRecordsArrayExtensionToBrowser(records);
    chrome.braveSync.resolvedSyncRecords(category_name, records);
  }
});

chrome.braveSync.onSendGetBookmarksBaseOrder.addListener(function(deviceId, platform) {
  console.log(`"get-bookmarks-base-order" deviceId=${JSON.stringify(deviceId)} platform=${JSON.stringify(platform)}`);
  callbackList["get-bookmarks-base-order"](null, deviceId, platform);
});

chrome.braveSync.onNeedSyncWords.addListener(function(seed) {
  var arr_int = seed.split(',').map(Number);
  var buffer = new Uint8Array(arr_int);
  var words = module.exports.passphrase.fromBytesOrHex(buffer, /*useNiceware=*/ false /* use bip39 */);
  console.log(`"NeedSyncWords" seed=${JSON.stringify(seed)} words=${JSON.stringify(words)}`);
  chrome.braveSync.syncWordsPrepared(words);
});

chrome.braveSync.onLoadClient.addListener(function() {
  console.log("in chrome.braveSync.onLoadClient");
  LoadJsLibScript();
});

chrome.braveSync.onClearOrderMap.addListener(function() {
  orderMap = {};
});

chrome.braveSync.extensionInitialized();
console.log("chrome.braveSync.extensionInitialized");

//-------------------------------------------------------------

function getOrder(record) {
  if ('bookmark' in record) {
    if (!record.bookmark.order) {
      getBookmarkOrderCallback = (order) => {
        record.bookmark.order = order;
        if (record.objectId)
          orderMap[record.objectId] = order;
        getBookmarkOrderCallback = null;
      }

      var prevOrder = record.bookmark.prevOrder;
      var parentOrder = record.bookmark.parentOrder;
      if (!prevOrder && orderMap[record.bookmark.prevObjectId])
        prevOrder = orderMap[record.bookmark.prevObjectId];
      if (!parentOrder && orderMap[record.bookmark.parentFolderObjectId])
        parentOrder = orderMap[record.bookmark.parentFolderObjectId];
      console.log(`"get-bookmark-order" prevOrder=${prevOrder}` +
        ` nextOrder=${record.bookmark.nextOrder} parentOrder=${parentOrder}`);
      callbackList["get-bookmark-order"](null, prevOrder,
        record.bookmark.nextOrder, parentOrder);
      while(getBookmarkOrderCallback);
    }
  }
}

function removeLocalMeta(record) {
  if ('bookmark' in record) {
    if ('prevObjectId' in record.bookmark) {
      delete record.bookmark.prevObjectId;
    }
    if ('prevOrder' in record.bookmark) {
      delete record.bookmark.prevOrder;
    }
    if ('nextOrder' in record.bookmark) {
      delete record.bookmark.nextOrder;
    }
    if ('parentOrder' in record.bookmark) {
      delete record.bookmark.parentOrder;
    }
  }
}

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
      if (sync_record.bookmark.parentFolderObjectIdStr) {
        sync_record.bookmark.parentFolderObjectId = new Uint8Array(IntArrayFromString(sync_record.bookmark.parentFolderObjectIdStr));
      }
      delete sync_record.bookmark.parentFolderObjectIdStr;
    }
    if ('prevObjectId' in sync_record.bookmark) {
      if (Array.isArray(sync_record.bookmark.prevObjectId)) {
        sync_record.bookmark.prevObjectId = new Uint8Array(sync_record.bookmark.prevObjectId);
      } else if (sync_record.bookmark.prevObjectId == null || sync_record.bookmark.prevObjectId.length == 0) {
        sync_record.bookmark.prevObjectId = new Uint8Array();
      }
    } else {
      sync_record.bookmark.prevObjectId = new Uint8Array();
    }
    if ('prevObjectIdStr' in sync_record.bookmark) {
      if (sync_record.bookmark.prevObjectIdStr) {
        sync_record.bookmark.prevObjectId = new Uint8Array(IntArrayFromString(sync_record.bookmark.prevObjectIdStr));
      }
      delete sync_record.bookmark.prevObjectIdStr;
    }
  }

}

function fixupSyncRecordsArrayExtensionToBrowser(records) {
  for(var i = 0; i < records.length; ++i) {
    fixupSyncRecordExtToBrowser(records[i]);
  }
}

class InjectedObject {
  handleMessage(message, arg1, arg2, arg3, arg4) {
    switch (message) {
      case "get-init-data":
        console.log(`"get-init-data" syncVersion=${JSON.stringify(arg1)}`);
        chrome.braveSync.getInitData(arg1/*syncVersion*/);
        break;
      case "sync-setup-error":
        console.log(`"sync-setup-error" error=${arg1}`);
        var errorToPass = (arg1 === 'Credential server response 400. Signed request body of the client timestamp is required.') ?
            'ERR_SYNC_REQUIRES_CORRECT_TIME' : 'ERR_SYNC_INIT_FAILED'
        chrome.braveSync.syncSetupError(errorToPass);
        break;
      case "sync-debug":
        console.log(`"sync-debug" message=${JSON.stringify(arg1)}`);
        chrome.braveSync.syncDebug(arg1/*message*/);
        break;
      case "save-init-data":
        var deviceId = arg2;
        if ( typeof deviceId == "number"  ) {
          deviceId = new Uint8Array([deviceId]);
        }
        if (!arg1) {
          arg1 = null;
        }
        console.log(`"save-init-data" seed=${JSON.stringify(arg1)} deviceId=${JSON.stringify(deviceId)}`);
        chrome.braveSync.saveInitData(arg1/*seed*/, deviceId);
        break;
      case "sync-ready":
        console.log(`"sync-ready"`);
        chrome.braveSync.syncReady();
        break;
      case "get-existing-objects":
        fixupBookmarkParentFolderObjectId(arg1, arg2);
        console.log(`"get-existing-objects" category_name=${JSON.stringify(arg1)} records=${JSON.stringify(arg2)} lastRecordTimeStamp=${arg3 ? arg3 : 0} isTruncated=${arg4 != undefined ? arg4 : false} `);
        chrome.braveSync.getExistingObjects(arg1/*category_name*/,
          arg2/*records*/, arg3 ? arg3 : 0/*lastRecordTimeStamp*/, arg4 != undefined ? arg4 : false/*isTruncated*/);
        break;
      case "resolved-sync-records":
        fixupSyncRecordsArrayExtensionToBrowser(arg2);
        console.log(`"resolved-sync-records" categoryName=${JSON.stringify(arg1)} records=${JSON.stringify(arg2)}`);
        chrome.braveSync.resolvedSyncRecords(arg1/*categoryName*/, arg2/*records*/);
        break;
      case "save-bookmarks-base-order":
        console.log(`"save-bookmarks-base-order" order=${JSON.stringify(arg1)} `);
        chrome.braveSync.saveBookmarksBaseOrder(arg1/*order*/);
        break;
      case "save-bookmark-order":
        console.log(`"save-bookmark-order" order=${JSON.stringify(arg1)} prevOrder=${JSON.stringify(arg2)} nextOrder=${JSON.stringify(arg3)} parentOrder=${JSON.stringify(arg4)}`);
        if (getBookmarkOrderCallback) {
          getBookmarkOrderCallback(arg1);
        }
        break;
      default:
        console.log('background.js TAGAB InjectedObject.handleMessage unknown message', message, arg1, arg2, arg3, arg4);
        console.log('background.js TAGAB message=' + message);
        console.log('background.js TAGAB arg1=' + arg1);
        console.log('background.js TAGAB arg2=' + arg2);
        console.log('background.js TAGAB arg3=' + arg3);
        console.log('background.js TAGAB arg4=' + arg4);
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
  var sync_bundle_url = chrome.runtime.getURL('extension/brave-sync/bundles/bundle.js');
  dynamicallyLoadScript(sync_bundle_url);
}

var callbackList = {}; /* message name to callback function */

var getBookmarkOrderCallback = null;
var orderMap = {}

if (!self.chrome) {
  self.chrome = {};
}

if(!self.chrome.ipc) {
  var ipc = {};

  ipc.once = function (message, cb) {
    callbackList[message] = cb;
    // injectedObject.handleMessage(message, '0', '0', '', false); // seems I should not call it. Just save the fn here
  };

  ipc.on = ipc.once;

  ipc.send = function (message, arg1, arg2, arg3, arg4) {
    injectedObject.handleMessage(message, arg1, arg2, arg3, arg4);
  };

  self.chrome.ipc = ipc;
  chrome.ipcRenderer = chrome.ipc;
}
