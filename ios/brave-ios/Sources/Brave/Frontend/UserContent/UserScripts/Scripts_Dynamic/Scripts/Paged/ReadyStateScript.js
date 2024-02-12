// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.execute(function($) {
  let postMessage = $(function(message) {
    $.postNativeMessage('$<message_handler>', {
      "securityToken": SECURITY_TOKEN,
      "state": message
    });
  });

  // Listen for document ready state
  document.addEventListener('readystatechange', $((event) => {
    postMessage(document.readyState);
  }));

  // Listen for document load state
  window.addEventListener('load', $((event) => {
    postMessage("loaded");
  }));

  // Listen for history popped
  window.addEventListener('popstate', $((event) => {
    if (event.state) {
      // Run on the browser's next run-loop
      setTimeout($(() => {
        postMessage("popstate");
      }), 0);
    }
  }));

  // Listen for history pushed
  const pushState = History.prototype.pushState;
  History.prototype.pushState = $(function(state, unused, url) {
    pushState.call(this, state, unused, url);

    if (state) {
      // Run on the browser's next run-loop
      setTimeout($(() => {
        postMessage("pushstate");
      }), 0);
    }
  });

  // Listen for history replaced
  const replaceState = History.prototype.replaceState;
  History.prototype.replaceState = $(function(state, unused, url) {
    replaceState.call(this, state, unused, url);

    if (state) {
      // Run on the browser's next run-loop
      setTimeout($(() => {
        postMessage("replacestate");
      }), 0);
    }
  });
});
