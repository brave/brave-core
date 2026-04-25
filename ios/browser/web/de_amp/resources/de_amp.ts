// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWebKitMessageWithReply} from '//ios/web/public/js_messaging/resources/utils.js';

let timesToCheck = 20;
let intervalId = 0;

function checkForAmp(): void {
  const htmlElm = document.documentElement;
  const headElm = document.head;

  if (!headElm && !htmlElm) {
    if (--timesToCheck === 0) {
      window.clearInterval(intervalId);
    }
    return;
  }

  if (!htmlElm.hasAttribute('amp') && !htmlElm.hasAttribute('⚡')) {
    window.clearInterval(intervalId);
    return;
  }

  const canonicalLinkElm =
      document.querySelector('head > link[rel="canonical"][href^="http"]');
  if (!canonicalLinkElm) {
    if (--timesToCheck === 0) {
      window.clearInterval(intervalId);
    }
    return;
  }

  try {
    const destUrl = new URL(canonicalLinkElm.getAttribute('href')!);
    window.clearInterval(intervalId);

    if (window.location.href === destUrl.href ||
        !(destUrl.protocol === 'http:' || destUrl.protocol === 'https:')) {
      return;
    }

    sendWebKitMessageWithReply('DeAmpMessageHandler', {destURL: destUrl.href})
        .then((shouldRedirect: boolean) => {
          if (shouldRedirect) {
            window.location.replace(destUrl.href);
          }
        });
  } catch (_) {
    window.clearInterval(intervalId);
  }
}

intervalId = window.setInterval(checkForAmp, 250);
checkForAmp();
