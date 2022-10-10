// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
      node.$<tagUUID> = uuid_v4();
    }
  }
  
  function sendMessage(name, node, target, type, detected) {
    $(function() {
      $.postNativeMessage('$<message_handler>', {
        "securityToken": SECURITY_TOKEN,
        "name": name,
        "src": node.src,
        "pageSrc": window.location.href,
        "pageTitle": document.title,
        "mimeType": type,
        "duration": clamp_duration(target.duration),
        "detected": detected,
        "tagId": target.$<tagUUID>
      });
    })();
  }
  
  function notify(target, type, detected) {
    if (target) {
      var name = target.title;
      if (!name || name == "") {
        name = document.title;
      }
    
      if (!type || type == "") {
        if (node.constructor.name == 'HTMLVideoElement') {
          type = 'video';
        }

        if (node.constructor.name == 'HTMLAudioElement') {
          type = 'audio';
        }
        
        if (node.constructor.name == 'HTMLSourceElement') {
          if (node.closest('video')) {
            type = 'video'
          } else {
            type = 'audio'
          }
        }
      }
      
      if (target.src && target.src !== "") {
        tagNode(target);
        sendMessage(name, target, target, type, detected);
      }
      else {
        target.querySelectorAll('source').forEach(function(node) {
          if (node.src && node.src !== "") {
            if ((node.closest('video') === target) || (node.closest('audio') === target)) {
              tagNode(target);
              sendMessaage(name, node, target, type, detected);
            }
          }
        });
      }
    }
  }
  
  function setupLongPress() {
    Object.defineProperty(window.__firefox__, 'playlistLongPressed', {
      enumerable: false,
      configurable: false,
      writable: false,
      value:
      function(localX, localY, token) {
        if (token != SECURITY_TOKEN) {
          return;
        }
      
        function execute(page, offsetX, offsetY) {
          var target = page.document.elementFromPoint(localX - offsetX, localY - offsetY);
          var targetVideo = target ? target.closest("video") : null;
          var targetAudio = target ? target.closest("audio") : null;

          // Video or Audio might have some sort of overlay..
          // Like player controls for pause/play, etc..
          // So we search for video/audio elements relative to touch position.
          if (!targetVideo && !targetAudio) {
            var touchX = localX + (page.scrollX + offsetX);
            var touchY = localY + (page.scrollY + offsetY);
        
            var videoElements = page.document.querySelectorAll('video');
            for (element of videoElements) {
              var rect = element.getBoundingClientRect();
              var x = rect.left + (page.scrollX + offsetX);
              var y = rect.top + (page.scrollY + offsetY);
              var w = rect.right - rect.left;
              var h = rect.bottom - rect.top;
              
              if (touchX >= x && touchX <= (x + w) && touchY >= y && touchY <= (y + h)) {
                targetVideo = element;
                break;
              }
            }
            
            var audioElements = page.document.querySelectorAll('audio');
            for (element of audioElements) {
              var rect = element.getBoundingClientRect();
              var x = rect.left + (page.scrollX + offsetX);
              var y = rect.top + (page.scrollY + offsetY);
              var w = rect.right - rect.left;
              var h = rect.bottom - rect.top;
              
              if (touchX >= x && touchX <= (x + w) && touchY >= y && touchY <= (y + h)) {
                targetAudio = element;
                break;
              }
            }
            
            // No elements found nearby so do nothing..
            if (!targetVideo && !targetAudio) {
              return;
            }
          }
          
          // Elements found
          if (targetVideo) {
            tagNode(targetVideo);
            notify(targetVideo, 'video', false);
          }

          if (targetAudio) {
            tagNode(targetAudio);
            notify(targetAudio, 'audio', false);
          }
        }
        
        // Any videos in the current `window.document`
        // will have an offset of (0, 0) relative to the window.
        execute(window, 0, 0);
        
        // Any videos in a `iframe.contentWindow.document`
        // will have an offset of (0, 0) relative to its contentWindow.
        // However, it will have an offset of (X, Y) relative to the current window.
        for (frame of document.querySelectorAll('iframe')) {
          // Get the frame's bounds relative to the current window.
          var bounds = frame.getBoundingClientRect();
          execute(frame.contentWindow, bounds.left, bounds.top);
        }
      }
    });
  }
  
  // MARK: ---------------------------------------
  
  function setupDetector() {
    function getAllVideoElements() {
        return document.querySelectorAll('video');
    }

    function getAllAudioElements() {
        return document.querySelectorAll('audio');
    }
    
    function requestWhenIdleShim(fn) {
      fn.toString = function() {
          return "function(){\n\t[native code]\n}";
      }
      
      var start = Date.now()
      return setTimeout(function () {
        fn({
          didTimeout: false,
          timeRemaining: function () {
            return Math.max(0, 50 - (Date.now() - start))
          },
        })
      }, 2000);  // Resolution of 1000ms is fine for us.
    }

    function onReady(fn) {
        fn.toString = function() {
            return "function(){\n\t[native code]\n}";
        }
      
        if (document.readyState === "complete" || document.readyState === "ready") {
            setTimeout(fn, 1);
        } else {
            document.addEventListener("DOMContentLoaded", fn);
        }
    }
    
    function observePage() {
      Object.defineProperty(HTMLMediaElement.prototype, '$<tagUUID>', {
        enumerable: false,
        configurable: false,
        writable: true,
        value: null
      });
    
      let observeNode = function(node) {
        // Observe video or audio elements
        let isVideoElement = (node.constructor.name == "HTMLVideoElement");
        if (isVideoElement || (node.constructor.name == "HTMLAudioElement")) {
          let type = isVideoElement ? 'video' : 'audio';
          node.observer = new MutationObserver(function (mutations) {
            notify(node, type, true);
          });
        
          node.observer.observe(node, { attributes: true, attributeFilter: ["src"] });
          node.addEventListener('loadedmetadata', function() {
            notify(node, type, true);
          });
        
          notify(node, type, true);
        }
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
    
      /*var document_createElement = document.createElement;
      document.createElement = function (tag) {
          if (tag === 'audio' || tag === 'video') {
              var node = document_createElement.call(this, tag);
              observeNode(node);
              notify(node, tag, true);
              return node;
          }
          return document_createElement.call(this, tag);
      };

      document.createElement.toString = function() {
          return "function () { [native code] }";
      };*/
    }

    observePage();
  }
  
  function setupTagNode() {
      Object.defineProperty(window.__firefox__, 'mediaCurrentTimeFromTag', {
        enumerable: false,
        configurable: false,
        writable: false,
        value:
        function(tag, token) {
          if (token != SECURITY_TOKEN) {
            return;
          }
        
          for (element of document.querySelectorAll('video')) {
            if (element.$<tagUUID> == tag) {
              return clamp_duration(element.currentTime);
            }
          }
          
          for (element of document.querySelectorAll('audio')) {
            if (element.$<tagUUID> == tag) {
              return clamp_duration(element.currentTime);
            }
          }
          
          return 0.0;
        }
      });
      
      Object.defineProperty(window.__firefox__, 'stopMediaPlayback', {
          enumerable: false,
          configurable: false,
          writable: false,
          value:
          function(token) {
            if (token != SECURITY_TOKEN) {
              return;
            }
          
            for (element of document.querySelectorAll('video')) {
              element.pause();
            }
            
            for (element of document.querySelectorAll('audio')) {
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
