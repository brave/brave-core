// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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
        } else if (node instanceof HTMLElement) {
          // Some sites inject a container element that already has video
          // descendants, so the video itself is never a direct added node.
          node.querySelectorAll('video').forEach(function(video) {
            video.addEventListener('pause', mediaPaused, false);
            video.addEventListener('playing', videoStateChanged, false);
            video.addEventListener('volumechange', videoStateChanged, false);
          });
        }
      }

      function isPlayingVideoWithAudio(video) {
        return !video.paused && !video.muted;
      }

      function mediaPaused() {
        sendMessage(false)
      }

      function videoStateChanged(event) {
        sendMessage(isPlayingVideoWithAudio(event.target))
      }

      function getVideoElements() {
        return document.querySelectorAll('video')
      }

      function hookVideoFunctions() {
        getVideoElements().forEach(function (item) {
          item.addEventListener('pause', mediaPaused, false);
          item.addEventListener('playing', videoStateChanged, false);
          item.addEventListener('volumechange', videoStateChanged, false);
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
