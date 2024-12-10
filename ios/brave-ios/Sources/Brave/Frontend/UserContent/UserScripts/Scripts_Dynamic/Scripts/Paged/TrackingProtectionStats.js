// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

"use strict";

window.__firefox__.execute(function($) {
  const messageHandler = '$<message_handler>';
  let sendInfo = [];
  let sendInfoTimeout = null;

  let sendMessage = $(function(urlString, resourceType) {
    // String is empty, null, undefined, ...
    if (!urlString) {
      return;
    }

    let resourceURL = null;
    try {
      resourceURL = new URL(urlString, document.location.href);

      // First party urls or invalid URLs are not blocked
      if (document.location.host === resourceURL.host) {
        return;
      }
    } catch (error) {
      console.error(error);
      return;
    }

    sendInfo.push({
      resourceURL: resourceURL.href,
      sourceURL: $.windowOrigin,
      resourceType: resourceType
    });

    if (sendInfoTimeout) {
      return;
    }

    // Send the URLs in batches every 500ms to avoid perf issues
    // from calling js-to-native too frequently.
    sendInfoTimeout = setTimeout($(() => {
      sendInfoTimeout = null;
      if (sendInfo.length == 0) {
        return;
      }

      $.postNativeMessage(messageHandler, {
        "securityToken": SECURITY_TOKEN,
        "data": sendInfo
      });

      sendInfo = [];
    }), 500);
  });

  let onLoadNativeCallback = $(function() {
    // Send back the sources of every script and image in the DOM back to the host application.
    [].slice.apply(document.scripts).forEach((el) => { sendMessage(el.src, "script"); });
    [].slice.apply(document.images).forEach((el) => { sendMessage(el.src, "image"); });
    [].slice.apply(document.getElementsByTagName('subdocument')).forEach((el) => { sendMessage(el.src, "subdocument"); })
  });

  let originalXHROpen = null;
  let originalXHRSend = null;
  let originalFetch = null;
  let originalImageSrc = null;
  let mutationObserver = null;

  let injectStatsTracking = $(function(enabled) {
    // This enable/disable section is a change from the original Focus iOS version.
    if (enabled) {
      if (originalXHROpen) {
        return;
      }
      window.addEventListener("load", onLoadNativeCallback, false);
    } else {
      window.removeEventListener("load", onLoadNativeCallback, false);

      if (originalXHROpen) { // if one is set, then all the enable code has run
        XMLHttpRequest.prototype.open = originalXHROpen;
        XMLHttpRequest.prototype.send = originalXHRSend;
        window.fetch = originalFetch;
        // Image.prototype.src = originalImageSrc;  // doesn't work to reset
        mutationObserver.disconnect();

        originalXHROpen = null;
        originalXHRSend = null;
        originalFetch = null;
        originalImageSrc = null;
        mutationObserver = null;
      }
      return;
    }

    // -------------------------------------------------
    // Send XHR requests URLs to the host application
    // -------------------------------------------------
    const localURLProp = Symbol('url')
    const localErrorHandlerProp = Symbol('tpErrorHandler')

    if (!originalXHROpen) {
      originalXHROpen = XMLHttpRequest.prototype.open;
      originalXHRSend = XMLHttpRequest.prototype.send;
    }

    XMLHttpRequest.prototype.open = $(function(method, url, isAsync) {
      // Blocked async XMLHttpRequest are handled via RequestBlocking.js
      // We only handle sync requests
      if (isAsync === undefined || isAsync) {
        return originalXHROpen.apply(this, arguments);
      }

      this[localURLProp] = url;
      return originalXHROpen.apply(this, arguments);
    }, /*overrideToString=*/false);

    XMLHttpRequest.prototype.send = $(function(body) {
      let url = this[localURLProp];
      if (!url) {
        return originalXHRSend.apply(this, arguments);
      }

      // Only attach the `error` event listener once for this
      // `XMLHttpRequest` instance.
      if (!this[localErrorHandlerProp]) {
        // If this `XMLHttpRequest` instance fails to load, we
        // can assume it has been blocked.
        this[localErrorHandlerProp] = $(function() {
          sendMessage(url, "xmlhttprequest");
        });

        this.addEventListener("error", this[localErrorHandlerProp]);
      }
      return originalXHRSend.apply(this, arguments);
    }, /*overrideToString=*/false);



    // -------------------------------------------------
    // Send `fetch()` request URLs to the host application
    // -------------------------------------------------
    if (!originalFetch) {
      originalFetch = window.fetch;
    }

    window.fetch = $(function(input, init) {
      if (typeof input === 'string') {
        sendMessage(input, 'xmlhttprequest');
      } else if (input instanceof Request) {
        sendMessage(input.url, 'xmlhttprequest');
      }

      return originalFetch.apply(window, arguments);
    }, /*overrideToString=*/false);

    // -------------------------------------------------
    // Detect when new sources get set on Image and send them to the host application
    // -------------------------------------------------
    if (!originalImageSrc) {
      originalImageSrc = Object.getOwnPropertyDescriptor(Image.prototype, "src");
    }

    delete Image.prototype.src;

    Object.defineProperty(Image.prototype, "src", {
      get: $(function() {
        return originalImageSrc.get.call(this);
      }),

      set: $(function(value) {
        // Only attach the `error` event listener once for this
        // Image instance.
        if (!this[localErrorHandlerProp]) {
          // If this `Image` instance fails to load, we can assume
          // it has been blocked.
          this[localErrorHandlerProp] = $(function() {
            sendMessage(this.src, "image");
          });

          this.addEventListener("error", this[localErrorHandlerProp]);
        }

        originalImageSrc.set.call(this, value);
      }),
      enumerable: true,
      configurable: true
    });

    // -------------------------------------------------
    // Listen to when new <script> elements get added to the DOM
    // and send the source to the host application
    // -------------------------------------------------
    mutationObserver = new MutationObserver($(function(mutations) {
      mutations.forEach($(function(mutation) {
        mutation.addedNodes.forEach($(function(node) {
          // `<script src="*">` elements.
          if (node.tagName === "SCRIPT" && node.src) {
            sendMessage(node.src, "script");
            return;
          }

          if (node.tagName === "IMG" && node.src) {
            sendMessage(node.src, "image");
            return;
          }

          // `<iframe src="*">` elements where [src] is not "about:blank".
          if (node.tagName === "IFRAME" && node.src) {
            if (node.src === "about:blank") {
              return;
            }

            sendMessage(node.src, "subdocument");
            return;
          }
        }));
      }));
    }));

    mutationObserver.observe(document.documentElement, {
      childList: true,
      subtree: true
    });
  });

  injectStatsTracking(true);
});
