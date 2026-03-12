// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWebKitMessage} from '//ios/web/public/js_messaging/resources/utils.js';

function sendMessage(playing: boolean) {
  sendWebKitMessage('AdsMediaReportingMessageHandler', {'isPlaying': playing});
}

function hookVideoElement(video: Element) {
  video.addEventListener('pause', () => sendMessage(false), false);
  video.addEventListener('playing', () => sendMessage(true), false);
}

document.querySelectorAll('video').forEach(hookVideoElement);

const observer = new MutationObserver(function(mutations: MutationRecord[]) {
  mutations.forEach(function(mutation: MutationRecord) {
    mutation.addedNodes.forEach(function(node: Node) {
      if (node instanceof HTMLVideoElement) {
        hookVideoElement(node);
      }
    });
  });
});
observer.observe(document, {subtree: true, childList: true});
