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
      // Identifies which video element a message is about, since a page can
      // have more than one playing at once.
      var nextPlayerId = 0;
      var playerIds = new WeakMap();

      function getPlayerId(video) {
        var playerId = playerIds.get(video);
        if (playerId === undefined) {
          playerId = nextPlayerId++;
          playerIds.set(video, playerId);
        }
        return playerId;
      }

      function sendMessage(playerId, playing) {
        webkit.messageHandlers.adsMediaReporting.postMessage({"securityToken": SECURITY_TOKEN, "data": {playerId, playing}});
      }

      function checkVideoNode(node) {
        if (node.constructor.name == "HTMLVideoElement") {
          hookVideoElement(node);
        } else if (node instanceof HTMLElement) {
          // Some sites inject a container element that already has video
          // descendants, so the video itself is never a direct added node.
          node.querySelectorAll('video').forEach(hookVideoElement);
        }
      }

      function isPlayingVideoWithAudio(video) {
        return !video.paused && !video.muted;
      }

      function mediaPaused(event) {
        sendMessage(getPlayerId(event.target), false)
      }

      function videoStateChanged(event) {
        sendMessage(getPlayerId(event.target), isPlayingVideoWithAudio(event.target))
      }

      function getVideoElements() {
        return document.querySelectorAll('video')
      }

      function hookVideoElement(video) {
        video.addEventListener('pause', mediaPaused, false);
        video.addEventListener('playing', videoStateChanged, false);
        video.addEventListener('volumechange', videoStateChanged, false);
      }

      function hookVideoFunctions() {
        getVideoElements().forEach(hookVideoElement);
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
