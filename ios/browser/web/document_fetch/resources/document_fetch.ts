// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrWebApi, gCrWeb}
    from '//ios/web/public/js_messaging/resources/gcrweb.js';
import {sendWebKitMessage}
    from '//ios/web/public/js_messaging/resources/utils.js';

function download(url: string): void {
  const xhr = new XMLHttpRequest();
  xhr.responseType = 'arraybuffer';
  xhr.onreadystatechange = function() {
    if (this.readyState !== XMLHttpRequest.DONE) {
      return;
    }
    if (this.status === 200) {
      const byteArray = new Uint8Array(this.response as ArrayBuffer);
      const binaryString =
          Array.from(byteArray).map(b => String.fromCharCode(b)).join('');
      sendWebKitMessage('DocumentFetchMessageHandler', {
        statusCode: this.status,
        base64Data: window.btoa(binaryString),
      });
    } else {
      sendWebKitMessage('DocumentFetchMessageHandler', {
        statusCode: this.status,
        base64Data: '',
      });
    }
  };
  xhr.open('GET', url, true);
  xhr.send(null);
}

const documentFetchApi = new CrWebApi('documentFetch');
documentFetchApi.addFunction('download', download);
gCrWeb.registerApi(documentFetchApi);
