// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// clang-format off
// #import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';
// clang-format on

cr.define('settings', function() {

  class BraveSyncBrowserProxy {
    getSyncCode() {
      return cr.sendWithPromise('SyncSetupGetSyncCode');
    }
    getQRCode(syncCode) {
      return cr.sendWithPromise('SyncGetQRCode', syncCode);
    }
    getDeviceList() {
      return cr.sendWithPromise('SyncGetDeviceList');
    }
    setSyncCode(syncCode) {
      return cr.sendWithPromise('SyncSetupSetSyncCode', syncCode);
    }
    resetSyncChain() {
      return cr.sendWithPromise('SyncSetupReset');
    }
  }

  cr.addSingletonGetter(BraveSyncBrowserProxy);

  // #cr_define_end
  return {
    BraveSyncBrowserProxy,
  };
});
