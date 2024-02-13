// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

"use strict";

window.__firefox__.includeOnce("RewardsReporting", function($) {
  const mediaPublisherOrigins = [
    'https://www.twitch.tv', 'https://m.twitch.tv', 'https://player.twitch.tv',
    'https://www.youtube.com', 'https://m.youtube.com',
    'https://vimeo.com',
  ]

  const install = () => {
    const sendMessage = $(function(method, url, data, referrerUrl) {
      $.postNativeMessage('$<message_handler>', {"securityToken": SECURITY_TOKEN, "data": {
        method: method === undefined ? "GET" : method,
        url: url,
        data: (data === undefined || data instanceof Blob) ? null : data,
        referrerUrl: referrerUrl === undefined ? null : referrerUrl,
      }});
    })

    const originalOpen = XMLHttpRequest.prototype.open;
    const originalSend = XMLHttpRequest.prototype.send;
    const originalFetch = window.fetch;
    const originalSendBeacon = navigator.sendBeacon;
    const originalImageSrc = Object.getOwnPropertyDescriptor(Image.prototype, "src");
    const localURLProp = Symbol('url')
    const localMethodProp = Symbol('method')
    const localRefProp = Symbol('ref')
    const localDataProp = Symbol('data')

    XMLHttpRequest.prototype.open = $(function(method, url) {
        const listener = function() {
            sendMessage(this[localMethodProp], this.responseURL === null ? this[localURLProp] : this.responseURL, this[localDataProp], this[localRefProp]);
        };
        this[localMethodProp] = method;
        this[localURLProp] = url;
        this.addEventListener('load', listener, true);
        this.addEventListener('error', listener, true);
        return originalOpen.apply(this, arguments);
    }, /*overrideToString=*/false);

    XMLHttpRequest.prototype.send = $(function(body) {
        this[localRefProp] = null;
        this[localDataProp] = body;
        if (body instanceof Document) {
            this[localRefProp] = body.referrer;
            this[localDataProp] = null;
        }
        return originalSend.apply(this, arguments);
    }, /*overrideToString=*/false);

    window.fetch = $(function(resource, options) {
        const args = arguments
        const url = resource instanceof Request ? resource.url : resource
        const method = options != null ? options.method : 'GET'
        const body = options != null ? options.body : null
        const referrer = options != null ? options.referrer : null

        return new Promise($(function(resolve, reject) {
          originalFetch.apply(this, args)
            .then(function(response) {
              sendMessage(method, url, body, referrer);
              resolve(response);
            })
            .catch(function(error) {
              sendMessage(method, url, body, referrer);
              reject(error);
            })
        }));
    }, /*overrideToString=*/false);

    navigator.sendBeacon = $(function(url, data) {
      sendMessage("POST", url, data);
      return originalSendBeacon.apply(this, arguments);
    });

    delete Image.prototype.src;
    Object.defineProperty(Image.prototype, "src", {
      get: $(function() {
        return originalImageSrc.get.call(this);
      }),
      set: $(function(value) {
        const listener = $(function() {
          sendMessage("GET", this.src);
        });

        this.addEventListener('load', listener, true);
        this.addEventListener('error', listener, true);
        originalImageSrc.set.call(this, value);
      }),
      enumerable: true,
      configurable: true
    });
  }

  if (mediaPublisherOrigins.includes(document.location.origin) && webkit.messageHandlers.$<message_handler>) {
    install();
  }
});
