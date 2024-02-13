// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// The below is needed because the script may not be web-packed into a bundle so it may be missing the run-once code

window.__firefox__.includeOnce("MediaBackgrounding", function($) {
  var descriptor = Object.getOwnPropertyDescriptor(Document.prototype, "visibilityState");
  var visibilityState_Get = descriptor.get;
  var visibilityState_Set = descriptor.set;
  Object.defineProperty(Document.prototype, 'visibilityState', {
    enumerable: descriptor.enumerable,
    configurable: descriptor.configurable,
    get: $(function() {
        var result = visibilityState_Get.call(this);
        if (result != "visible") {
            return "visible";
        }
        return result;
    }),
    set: $(function(value) {
      visibilityState_Set.call(this, value);
    })
  });

  Object.defineProperty(HTMLVideoElement.prototype, 'userHitPause', {
    enumerable: false,
    configurable: false,
    writable: true,
    value: false
  });

  Object.defineProperty(HTMLVideoElement.prototype, 'pauseListener', {
    enumerable: false,
    configurable: false,
    writable: true,
    value: false
  });

  Object.defineProperty(HTMLVideoElement.prototype, 'presentationModeListener', {
    enumerable: false,
    configurable: false,
    writable: true,
    value: false
  });

  var pauseControl = HTMLVideoElement.prototype.pause;
  HTMLVideoElement.prototype.pause = $(function() {
    this.userHitPause = true;
    return pauseControl.call(this);
  });

  var playControl = HTMLVideoElement.prototype.play;
  HTMLVideoElement.prototype.play = $(function() {
    this.userHitPause = false;
    return playControl.call(this);
  });

  let addListeners = $(function(element) {
    if (!element.pauseListener) {
      element.pauseListener = true;
      element.visibilityState = visibilityState_Get.call(document);

      document.addEventListener("visibilitychange", $(function(e) {
        element.visibilityState = visibilityState_Get.call(document);
      }), false);

      element.addEventListener("pause", $(function(e) {
        if (!element.userHitPause && visibilityState_Get.call(document) == "visible") {
          var onVisibilityChanged = $((e) => {
            document.removeEventListener("visibilitychange", onVisibilityChanged);

            if (visibilityState_Get.call(document) != "visible" && !element.ended) {
              playControl.call(element);
            }
          });

          document.addEventListener("visibilitychange", onVisibilityChanged);

          setTimeout($(function() {
            document.removeEventListener("visibilitychange", onVisibilityChanged);
          }), 2000);
        } else {
          if (!element.userHitPause && element.visibilityState == "visible" && !element.ended) {
            playControl.call(element);
          }
        }
      }), false);
    }

    if (!element.presentationModeListener) {
        element.presentationModeListener = true;

        element.addEventListener('webkitpresentationmodechanged', $(function(e) {
          e.stopPropagation();
        }), true);
    }
  });

  const queue = [];
  let onMutation = $(function() {
    for (const mutations of queue) {
      mutations.addedNodes.forEach(function (node) {
        if (node.constructor.name == 'HTMLVideoElement') {
          addListeners(node);
        }
      });
    }
    queue.length = 0;
  });

  var observer = new MutationObserver($(function(mutations) {
    if (!queue.length) {
      // Debounce the mutation for performance
      // with setTimeout | requestIdleCallback | requestAnimationFrame
      // requestIdleCallback isn't available on iOS yet.
      requestAnimationFrame(onMutation);
    }
    queue.push(...mutations);
  }));

  observer.observe(document, {
    childList: true,
    attributes: false,
    characterData: false,
    subtree: true,
    attributeOldValue: false,
    characterDataOldValue: false
  });
});
