// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// MARK: - Media Detection

window.__firefox__.includeOnce("Playlist", function($) {
  function is_nan(value) {
    return typeof value === "number" && value !== value;
  }

  function is_infinite(value) {
    return typeof value === "number" && (value === Infinity || value === -Infinity);
  }

  function clamp_duration(value) {
    if (is_nan(value)) {
      return 0.0;
    }

    if (is_infinite(value)) {
      return Number.MAX_VALUE;
    }
    return value;
  }

  // Algorithm:
  // Generate a random number from 0 to 256
  // Roll-Over clamp to the range [0, 15]
  // If the index is 13, set it to 4.
  // If the index is 17, clamp it to [0, 3]
  // Subtract that number from 15 (XOR) and convert the result to hex.
  function uuid_v4() {
    // X >> 2 = X / 4 (integer division)

    // AND-ing (15 >> 0) roll-over clamps to 15
    // AND-ing (15 >> 2) roll-over clamps to 3
    // So '8' digit is clamped to 3 (inclusive) and all others clamped to 15 (inclusive).

    // 0 XOR 15 = 15
    // 1 XOR 15 = 14
    // 8 XOR 15 = 7
    // So N XOR 15 = 15 - N

    // UUID string format generated with array appending
    // Results in "10000000-1000-4000-8000-100000000000".replace(...)
    return ([1e7]+-1e3+-4e3+-8e3+-1e11).replace(/[018]/g, (X) => {
      return (X ^ (crypto.getRandomValues(new Uint8Array(1))[0] & (15 >> (X >> 2)))).toString(16);
    });
  }

  function tagNode(node) {
    if (node) {
      if (!node.$<tagUUID>) {
        node.addEventListener('webkitpresentationmodechanged', (e) => e.stopPropagation(), true);
      }

      // This is awful on dynamic websites.
      // Some websites are now using the re-using video tag even if the page itself, and the history changes.
      // I no longer have a proper way to detect if a video was already detected.
      // IE: If a page has TWO videos on it, there is no way to display which was already added, as we have to now rely on PageURL
      // or video src (or <source> tag)
      node.$<tagUUID> = uuid_v4();
    }
  }

  let sendMessage = $(function(name, node, target, type, detected) {
    $(function() {
      var location = "";
      var pageTitle = "";

      try {
        location = window.top.location.href;
        pageTitle = window.top.document.title;
      } catch(error) {
        location = window.location.href;
        pageTitle = document.title;
      }

      $.postNativeMessage('$<message_handler>', {
        "securityToken": SECURITY_TOKEN,
        "name": name,
        "src": node.src,
        "pageSrc": location,
        "pageTitle": pageTitle,
        "mimeType": type,
        "duration": clamp_duration(target.duration),
        "detected": detected,
        "tagId": target.$<tagUUID>,
        "invisible": !target.parentNode
      });
    })();
  });

  function isVideoNode(node) {
    return node.constructor.name === 'HTMLVideoElement' || node.tagName === 'VIDEO';
  }

  function isAudioNode(node) {
    return node.constructor.name === 'HTMLAudioElement' || node.tagName === 'AUDIO';
  }

  function isSourceNode(node) {
    return node.constructor.name === 'HTMLSourceElement' || node.tagName === "SOURCE";
  }

  function notifyNode(target, type, detected, ignoreSource) {
    if (target) {
      var name = target.title;
      if (!name || name == "") {
        try {
          name = window.top.document.title;
        } catch(error) {
          name = document.title;
        }
      }

      if (!type || type == "") {
        if (isVideoNode(target)) {
          type = 'video';
        }

        if (isAudioNode(target)) {
          type = 'audio';
        }

        if (isSourceNode(target)) {
          if (isVideoNode(target.parentNode)) {
            type = 'video'
          } else {
            type = 'audio'
          }
        }
      }

      if (ignoreSource || (target.src && target.src !== "")) {
        tagNode(target);
        sendMessage(name, target, target, type, detected);
      }
      else {
        for (node of target.children) {
          if (isSourceNode(node)) {
            if (node.src && node.src !== "") {
              tagNode(target);
              sendMessage(name, node, target, type, detected);
            }
          }
        }
      }
    }
  }

  function isElementVisible(e) {
    if (!!(e.offsetWidth && e.offsetHeight && e.getClientRects().length)) {
      return true;
    }

    var style = page.getComputedStyle(e);
    return style.width != 0 &&
           style.height !== 0 &&
           style.opacity > 0 &&
           style.display !== 'none' &&
           style.visibility !== 'hidden';
  }

  function getAllVideoElements() {
    return [...document.querySelectorAll('video')].reverse();
  }

  function getAllAudioElements() {
    return [...document.querySelectorAll('audio')].reverse();
  }

  function setupLongPress() {
    Object.defineProperty(window.__firefox__, '$<playlistLongPressed>', {
      enumerable: false,
      configurable: false,
      writable: false,
      value:
      function(localX, localY, token) {
        if (token != SECURITY_TOKEN) {
          return;
        }

        function execute(page, offsetX, offsetY) {
          var targets = page.document.elementsFromPoint(localX - offsetX, localY - offsetY).filter((e) => {
            return isVideoNode(e) || isAudioNode(e);
          }).filter((e) => {
            return isElementVisible(e);
          });


          if (targets.length == 0) {
            var targetAudio = page.document.querySelector('audio');
            if (targetAudio) {
              tagNode(targetAudio);
              notifyNode(targetAudio, 'audio', false, false);
            }
            return;
          }

          var targetVideo = targets[0];
          if (targetVideo) {
            tagNode(targetVideo);
            notifyNode(targetVideo, 'video', false, false);
          }
        }

        // Any videos in the current `window.document`
        // will have an offset of (0, 0) relative to the window.
        execute(window, 0, 0);

        // Any videos in a `iframe.contentWindow.document`
        // will have an offset of (0, 0) relative to its contentWindow.
        // However, it will have an offset of (X, Y) relative to the current window.
        try {
          for (frame of document.querySelectorAll('iframe')) {
            // Get the frame's bounds relative to the current window.
            var bounds = frame.getBoundingClientRect();
            execute(frame.contentWindow, bounds.left, bounds.top);
          }
        } catch {}
      }
    });
  }

  // MARK: ---------------------------------------

  function setupDetector() {
    function onReady(fn) {
      if (document.readyState === "complete" || document.readyState === "ready") {
        fn();
      } else {
        document.addEventListener("DOMContentLoaded", fn);
      }
    }

    function observePage() {
      let useObservers = false;

      Object.defineProperty(HTMLMediaElement.prototype, '$<tagUUID>', {
        enumerable: false,
        configurable: false,
        writable: true,
        value: null
      });

      if (useObservers) {
        let observeNode = function(node) {
          function processNode(node) {
            // Observe video or audio elements
            let isVideoElement = isVideoNode(node);
            let isAudioElement = isAudioNode(node);
            if (isVideoElement || isAudioElement) {
              let type = isVideoElement ? 'video' : 'audio';
              node.observer = new MutationObserver(function (mutations) {
                notifyNode(node, type, true, false);
              });

              node.observer.observe(node, { attributes: true, attributeFilter: ["src"] });
              node.addEventListener('loadedmetadata', function() {
                notifyNode(node, type, true, false);
              });

              notifyNode(node, type, true, false);
            }
          }

          for (const child of node.childNodes) {
            processNode(child);
          }

          processNode(node);
        };

        // Observe elements added to a Node
        let documentObserver = new MutationObserver(function (mutations) {
          mutations.forEach(function (mutation) {
            mutation.addedNodes.forEach(function (node) {
              observeNode(node);
            });
          });
        });

        documentObserver.observe(document, { subtree: true, childList: true });
      } else {
        Object.defineProperty(HTMLMediaElement.prototype, '$<tagUUID>', {
          enumerable: false,
          configurable: false,
          writable: true,
          value: null
        });

        var descriptor = Object.getOwnPropertyDescriptor(HTMLMediaElement.prototype, 'src');
        if (descriptor) {
          Object.defineProperty(HTMLMediaElement.prototype, 'src', {
            enumerable: descriptor.enumerable,
            configurable: descriptor.configurable,
            get: function() {
              return descriptor.get.call(this);
            },
            set: function(value) {
              descriptor.set.call(this, value);
              
              if (this instanceof HTMLVideoElement) {
                notifyNode(this, 'video', true, false);
              } else if (this instanceof HTMLAudioElement) {
                notifyNode(this, 'audio', true, false);
                setTimeout(() => checkPageForVideos(false), 100);
              }
            }
          });
        }

        var setVideoAttribute = HTMLVideoElement.prototype.setAttribute;
        HTMLVideoElement.prototype.setAttribute = function(key, value) {
          setVideoAttribute.call(this, key, value);
          if (key.toLowerCase() == 'src') {
            notifyNode(this, 'video', true, false);
          }
        };

        var setAudioAttribute = HTMLAudioElement.prototype.setAttribute;
        HTMLAudioElement.prototype.setAttribute = function(key, value) {
          setAudioAttribute.call(this, key, value);
          if (key.toLowerCase() == 'src') {
            notifyNode(this, 'audio', true, false);
            
            // Instead of using an interval and polling,
            // we can check the page after a short period when an audio source has been setup.
            setTimeout(() => checkPageForVideos(false), 100);
          }
        };
      }

      /*var documentCreateElement = document.createElement;
      document.createElement = function (tag) {
          if (tag === 'audio' || tag === 'video') {
              var node = documentCreateElement.call(this, tag);
              observeNode(node);
              notifyNode(node, tag, true, false);
              return node;
          }
          return documentCreateElement.call(this, tag);
      };*/

      function checkPageForVideos(ignoreSource) {
        function fetchMedia() {
          let videos = getAllVideoElements();
          let audios = getAllAudioElements();

          if (videos.length == 0 && audios.length == 0) {
            setTimeout(function() {
              $.postNativeMessage('$<message_handler>', {
                "securityToken": SECURITY_TOKEN,
                "state": "cancel"
              });
            }, 10000);
            return;
          }

          videos.forEach(function(node) {
            if (useObservers) {
              observeNode(node);
            }
            notifyNode(node, 'video', true, ignoreSource);
          });

          audios.forEach(function(node) {
            if (useObservers) {
              observeNode(node);
            }
            notifyNode(node, 'audio', true, ignoreSource);
          });
        }
        
        onReady(function() {
          fetchMedia();

          $(function() {
            $.postNativeMessage('$<message_handler>', {
              "securityToken": SECURITY_TOKEN,
              "state": document.readyState
            });
          })();
        });
        
        fetchMedia();
        
        // Do one last check (if the page took too long to load - DailyMotion)
        setTimeout(function() {
          fetchMedia();
        }, 5000);
      }

      // Needed for Japanese videos like tver.jp which literally never loads automatically
      Object.defineProperty(window.__firefox__, '$<playlistProcessDocumentLoad>', {
        enumerable: false,
        configurable: false,
        writable: false,
        value:
        function() {
          checkPageForVideos(true);
        }
      });

      if (useObservers) {
        // Needed for pages like Bichute and Soundcloud and Youtube that do NOT reload the page
        // They instead alter the history or document and update the href that way
        window.addEventListener("load", () => {
          let lastLocation = document.location.href;
          const body = document.querySelector("body");
          const observer = new MutationObserver(mutations => {
            if (lastLocation !== document.location.href) {
              lastLocation = document.location.href;
              checkPageForVideos(false);
            }
          });
          observer.observe(body, { childList: true, subtree: true });
        });
      }

      checkPageForVideos(false);
    }

    observePage();
  }

  function setupTagNode() {
      Object.defineProperty(window.__firefox__, '$<mediaCurrentTimeFromTag>', {
        enumerable: false,
        configurable: false,
        writable: false,
        value:
        function(tag, token) {
          if (token != SECURITY_TOKEN) {
            return;
          }

          for (const element of getAllVideoElements()) {
            if (element.$<tagUUID> == tag) {
              return clamp_duration(element.currentTime);
            }
          }

          for (const element of getAllAudioElements()) {
            if (element.$<tagUUID> == tag) {
              return clamp_duration(element.currentTime);
            }
          }

          return 0.0;
        }
      });

      Object.defineProperty(window.__firefox__, '$<stopMediaPlayback>', {
          enumerable: false,
          configurable: false,
          writable: false,
          value:
          function(token) {
            if (token != SECURITY_TOKEN) {
              return;
            }

            for (element of getAllVideoElements()) {
              element.pause();
            }

            for (element of getAllAudioElements()) {
              element.pause();
            }

            return 0.0;
          }
      });
  }

  // MARK: -----------------------------

  setupLongPress();
  setupDetector();
  setupTagNode();
});
