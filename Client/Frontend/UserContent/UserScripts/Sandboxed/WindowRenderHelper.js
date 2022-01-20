// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

Object.defineProperty(window, "$<windowRenderer>", {
  value: {}
})

Object.defineProperty($<windowRenderer>, "postMessage", {
  value: function (msg) {
      if (msg) {
          webkit.messageHandlers.$<handler>.postMessage(msg);
      }
  }
})

Object.defineProperty($<windowRenderer>, "resizeWindow", {
  value: function () {
      var evt = document.createEvent('UIEvents');
      evt.initUIEvent('resize', true, false, window, 0);
      window.dispatchEvent(evt);
  }
})

Object.defineProperty($<windowRenderer>, "addDocumentStateListener", {
  value: function () {
      document.addEventListener('readystatechange', event => {
          if (event.target.readyState === "interactive") {
              //Used for debugging in Safari development tools to know what the state of the page is.
              //Not needed while in use because we only care about JSON messages and not state.
              //postMessage(event.target.readyState);
              $<windowRenderer>.resizeWindow();
          }

          if (event.target.readyState === "complete") {
              //Used for debugging in Safari development tools to know what the state of the page is.
              //Not needed while in use because we only care about JSON messages and not state.
              //postMessage(event.target.readyState);
              $<windowRenderer>.resizeWindow();
          }
      });
  }
})

$<windowRenderer>.addDocumentStateListener()
