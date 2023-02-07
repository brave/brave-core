// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

"use strict";

window.__firefox__.execute(function($) {
  (function() {
    const messageHandler = '$<message_handler>';
    const W = window;
    const D = W.document;
    
    let timesToCheck = 20;
    let intervalId = 0;
    
    const sendMessage = $((destURL) => {
      return $.postNativeMessage(messageHandler, {
        "securityToken": SECURITY_TOKEN,
        "destURL": destURL.href
      });
    });
    
    const checkIfShouldStopChecking = $(_ => {
      timesToCheck -= 1;
      if (timesToCheck === 0) {
        W.clearInterval(intervalId);
      }
    });
    
    const checkForAmp = $(_ => {
      const htmlElm = document.documentElement;
      const headElm = document.head;
      
      if (!headElm && !htmlElm) {
        // checked too early and page structure not available.
        checkIfShouldStopChecking();
        return;
      }
      
      if (!htmlElm.hasAttribute('amp') && !htmlElm.hasAttribute('⚡')) {
        // We know this isn't an amp document, and no point of checking further
        W.clearInterval(intervalId);
        return;
      }
      
      const canonicalLinkElm = D.querySelector('head > link[rel="canonical"][href^="http"]')
      if (canonicalLinkElm === null || canonicalLinkElm === undefined) {
        // didn't find a link elm.
        checkIfShouldStopChecking();
        return;
      }
      
      const targetHref = canonicalLinkElm.getAttribute('href');
      try {
        const destUrl = new URL(targetHref);
        W.clearInterval(intervalId);
        
        if (W.location.href == destUrl.href || !(destUrl.protocol === 'http:' || destUrl.protocol === 'https:')) {
          // Only handle http/https and only if the canoncial url is different than the current url
          // Also add a check the referrer to prevent an infinite load loop in some cases
          return;
        }
        
        sendMessage(destUrl).then(deAmp => {
          if (deAmp) {
            W.location.replace(destUrl.href);
          }
        })
      } catch (_) {
        // Invalid canonical URL detected
        W.clearInterval(intervalId);
        return;
      }
    });
    
    intervalId = W.setInterval(checkForAmp, 250);
    checkForAmp();
  })();
});
