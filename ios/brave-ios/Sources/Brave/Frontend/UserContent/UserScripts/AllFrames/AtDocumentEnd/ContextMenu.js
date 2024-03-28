// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

"use strict";

// Ensure this module only gets included once. This is
// required for user scripts injected into all frames.
window.__firefox__.includeOnce("ContextMenu", function($) {
  window.addEventListener("touchstart", function(evt) {
    var target = evt.target;

    var targetImage = target.closest("img");

    if (!targetImage) {
      webkit.messageHandlers.contextMenuMessageHandler.postMessage({});
      return;
    }

    var data = {};

    data.touchX = evt.changedTouches[0].pageX - window.scrollX;
    data.touchY = evt.changedTouches[0].pageY - window.scrollY;

    if (targetImage) {
      data.image = targetImage.src;
      data.title = data.title || targetImage.title;
      data.alt = targetImage.alt;
    }

    if (data.image) {
      webkit.messageHandlers.contextMenuMessageHandler.postMessage(data);
    }
  }, true);
});
