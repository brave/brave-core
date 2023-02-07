// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

"use strict";

(function(){
    if (webkit.messageHandlers.adsMediaReporting) {
        install();
    }

    function install() {
      function sendMessage(playing) {
        webkit.messageHandlers.adsMediaReporting.postMessage({"securityToken": SECURITY_TOKEN, "data": {playing}});
      }

      function checkVideoNode(node) {
        if (node.constructor.name == "HTMLVideoElement") {
          hookVideoFunctions();
        }
      }

      function mediaPaused() {
        sendMessage(false)
      }

      function mediaPlaying() {
        sendMessage(true)
      }

      function getVideoElements() {
        return document.querySelectorAll('video')
      }

      function hookVideoFunctions() {
        getVideoElements().forEach(function (item) {
          item.addEventListener('pause', mediaPaused, false);
          item.addEventListener('playing', mediaPlaying, false);
        });
      }

      var observer = new MutationObserver(function (mutations) {
        mutations.forEach(function (mutation) {
          mutation.addedNodes.forEach(function (node) {
            checkVideoNode(node);
          });
        });
      });
      observer.observe(document, {subtree: true, childList: true });
    }
})()
