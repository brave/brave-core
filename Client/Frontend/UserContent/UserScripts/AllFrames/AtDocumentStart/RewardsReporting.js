/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const mediaPublisherOrigins = [
    'https://www.twitch.tv', 'https://m.twitch.tv', 'https://player.twitch.tv',
    'https://www.youtube.com', 'https://m.youtube.com',
    'https://vimeo.com',
]

if (mediaPublisherOrigins.includes(document.location.origin) && webkit.messageHandlers.rewardsReporting) {
    install();
}

function install() {
    function sendMessage(method, url, data, referrerUrl) {
        webkit.messageHandlers.rewardsReporting.postMessage({
            method: method === undefined ? "GET" : method,
            url: url,
            data: data === undefined ? null : data,
            referrerUrl: referrerUrl === undefined ? null : referrerUrl,
        });
    }
    
    let originalOpen = XMLHttpRequest.prototype.open;
    let originalSend = XMLHttpRequest.prototype.send;
    let originalFetch = window.fetch;
    let originalSendBeacon = navigator.sendBeacon;
    let originalImageSrc = Object.getOwnPropertyDescriptor(Image.prototype, "src");
    
    XMLHttpRequest.prototype.open = function(method, url) {
        const listener = function() {
            sendMessage(this._method, this.responseURL === null ? this._url : this.responseURL, this._data, this._ref);
        };
        this._method = method;
        this._url = url;
        this.addEventListener('load', listener, true);
        this.addEventListener('error', listener, true);
        return originalOpen.apply(this, arguments);
    };
    
    XMLHttpRequest.prototype.send = function(body) {
        this._ref = null;
        this._data = body;
        if (body instanceof Document) {
            this._ref = body.referrer;
            this._data = null;
        }
        return originalSend.apply(this, arguments);
    };

    window.fetch = function(resource, options) {
        const args = arguments
        const url = resource instanceof Request ? resource.url : resource
        const method = options != null ? options.method : 'GET'
        const body = options != null ? options.body : null
        const referrer = options != null ? options.referrer : null

        return new Promise(function(resolve, reject) {
            originalFetch.apply(this, args)
            .then(function(response) {
                sendMessage(method, url, body, referrer);
                resolve(response);
            })
            .catch(function(error) {
                sendMessage(method, url, body, referrer);
                reject(error);
            })
        });
    };

    navigator.sendBeacon = function(url, data) {
        sendMessage("POST", url, data);
        return originalSendBeacon.apply(this, arguments);
    };
    
    delete Image.prototype.src;
    Object.defineProperty(Image.prototype, "src", {
      get: function() {
        return originalImageSrc.get.call(this);
      },
      set: function(value) {
        const listener = function() {
            sendMessage("GET", this.src);
        };
        this.addEventListener('load', listener, true);
        this.addEventListener('error', listener, true);
        originalImageSrc.set.call(this, value);
      }
    });
}
