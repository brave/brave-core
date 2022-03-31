// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

(function() {
  // Listen for document ready state
  document.addEventListener('readystatechange', (event) => {
    window.webkit.messageHandlers.$<handler>.postMessage({
      "securitytoken": "$<security_token>",
      "state": document.readyState
    });
  });
  
  // Listen for document load state
  window.addEventListener('load', (event) => {
    window.webkit.messageHandlers.$<handler>.postMessage({
      "securitytoken": "$<security_token>",
      "state": "loaded"
    });
  });
  
  // Listen for history popped
  window.addEventListener('popstate', (event) => {
    if (event.state) {
      // Run on the browser's next run-loop
      setTimeout(() => {
        window.webkit.messageHandlers.$<handler>.postMessage({
          "securitytoken": "$<security_token>",
          "state": "popstate"
        });
      }, 0);
    }
  });
  
  // Listen for history pushed
  const pushState = History.prototype.pushState;
  History.prototype.pushState = function(state, unused, url) {
    pushState.call(this, state, unused, url);
    
    if (state) {
      // Run on the browser's next run-loop
      setTimeout(() => {
        window.webkit.messageHandlers.$<handler>.postMessage({
          "securitytoken": "$<security_token>",
          "state": "pushstate"
        });
      }, 0);
    }
  };
  
  // Listen for history replaced
  const replaceState = History.prototype.replaceState;
  History.prototype.replaceState = function(state, unused, url) {
    replaceState.call(this, state, unused, url);
    
    if (state) {
      // Run on the browser's next run-loop
      setTimeout(() => {
        window.webkit.messageHandlers.$<handler>.postMessage({
          "securitytoken": "$<security_token>",
          "state": "replacestate"
        });
      }, 0);
    }
  };
  
  // Hide the pushState trampoline
  History.prototype.pushState.toString = function() {
    return "function () { [native code]; }";
  };
  
  // Hide the replaceState trampoline
  History.prototype.replaceState.toString = function() {
      return "function () { [native code]; }";
  };
})();
