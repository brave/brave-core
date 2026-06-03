// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

function enable(): void {
  const descriptor = Object.getOwnPropertyDescriptor(
    Document.prototype, 'visibilityState'
  )!;
  const visibilityStateGet = descriptor.get!;

  Object.defineProperty(Document.prototype, 'visibilityState', {
    enumerable: descriptor.enumerable,
    configurable: descriptor.configurable,
    get() {
      const result = visibilityStateGet.call(this);
      if (result !== 'visible') {
        return 'visible';
      }
      return result;
    }
  });

  const pauseControl = HTMLVideoElement.prototype.pause;
  HTMLVideoElement.prototype.pause = function(): void {
    (this as any).userHitPause = true;
    pauseControl.call(this);
  };

  const playControl = HTMLVideoElement.prototype.play;
  HTMLVideoElement.prototype.play = function(): Promise<void> {
    (this as any).userHitPause = false;
    return playControl.call(this);
  };

  function addListeners(element: HTMLVideoElement): void {
    if (!(element as any).pauseListener) {
      (element as any).pauseListener = true;
      (element as any).visibilityState = visibilityStateGet.call(document);

      document.addEventListener('visibilitychange', function() {
        (element as any).visibilityState = visibilityStateGet.call(document);
      }, false);

      element.addEventListener('pause', function() {
        if (!(element as any).userHitPause &&
            visibilityStateGet.call(document) === 'visible') {
          const onVisibilityChanged = () => {
            document.removeEventListener(
              'visibilitychange', onVisibilityChanged
            );
            if (visibilityStateGet.call(document) !== 'visible' &&
                !element.ended) {
              playControl.call(element);
            }
          };
          document.addEventListener('visibilitychange', onVisibilityChanged);
          setTimeout(function() {
            document.removeEventListener(
              'visibilitychange', onVisibilityChanged
            );
          }, 2000);
        } else {
          if (!(element as any).userHitPause &&
              (element as any).visibilityState === 'visible' &&
              !element.ended) {
            playControl.call(element);
          }
        }
      }, false);
    }

    if (!(element as any).presentationModeListener) {
      (element as any).presentationModeListener = true;
      element.addEventListener('webkitpresentationmodechanged', function(e) {
        e.stopPropagation();
      }, true);
    }
  }

  const queue: MutationRecord[] = [];
  function onMutation(): void {
    for (const mutation of queue) {
      mutation.addedNodes.forEach(function(node: Node) {
        if (node instanceof HTMLVideoElement) {
          addListeners(node);
        }
      });
    }
    queue.length = 0;
  }

  const observer = new MutationObserver(function(mutations: MutationRecord[]) {
    if (!queue.length) {
      requestAnimationFrame(onMutation);
    }
    queue.push(...mutations);
  });

  document.querySelectorAll('video').forEach(
    (v) => addListeners(v as HTMLVideoElement)
  );

  observer.observe(document, {
    childList: true,
    attributes: false,
    characterData: false,
    subtree: true,
    attributeOldValue: false,
    characterDataOldValue: false
  });
}

if ((window as any).gCrWebPlaceholderMediaBackgroundingEnabled) {
  enable();
}
