// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

"use strict";

window.__firefox__.execute(function($) {
  const messageHandler = '$<message_handler>';
  const blockingCache = new Map();
  const sendMessage = $((resourceURL) => {
    // when location.href does not match origin, we include origin in console
    const originDisplay = window.location.href !== window.origin ? ` (${window.origin})` : ``;
    if (blockingCache.has(resourceURL.href)) {
      return Promise.resolve(blockingCache.get(resourceURL.href)).then(blocked => {
        if (blocked) {
          console.info(`Brave prevented frame displaying ${window.location.href}${originDisplay} from loading a resource from ${resourceURL.href} (cached)`)
        }
        return blocked
      })
    }

    return $.postNativeMessage(messageHandler, {
      "securityToken": SECURITY_TOKEN,
      "data": {
        resourceURL: resourceURL.href,
        windowLocationHref: window.location.href,
        windowOrigin: window.origin,
        resourceType: 'xmlhttprequest'
      }
    }).then(blocked => {
      blockingCache.set(resourceURL.href, blocked)

      if (blocked) {
        console.info(`Brave prevented frame displaying ${window.location.href}${originDisplay} from loading a resource from ${resourceURL.href}`)
      }

      return blocked
    });
  });
  const patchProgressEvent = $((progressEvent) =>{
    Object.setPrototypeOf(progressEvent, window.XMLHttpRequestProgressEvent.prototype)
    Object.defineProperties(progressEvent, {
      position: { value: 0 },
      totalSize: { value: 0 }
    })
    return progressEvent
  });

  const { fetch: originalFetch } = window
  window.fetch = $(function() {
    const [resource] = arguments

    // Extract the url
    let urlString
    if (typeof resource === 'string') {
      // Simple send where resource is a string
      urlString = resource
    } else if (resource instanceof Request) {
      // Assume it's a Request
      urlString = resource.url
    } else {
      return originalFetch.apply(this, arguments)
    }

    const url = new URL(urlString, window.location.href)
    return sendMessage(url).then(blocked => {
      if (blocked) {
        return Promise.reject(new TypeError('Load failed'))
      } else {
        return originalFetch.apply(this, arguments)
      }
    })
  }, /*overrideToString=*/false);

  const localURLProp = Symbol('url')
  const originalOpen = XMLHttpRequest.prototype.open
  XMLHttpRequest.prototype.open = $(function() {
    // Check if we're async
    const isAsync = arguments[2]
    if (isAsync !== undefined && !isAsync) {
      // We can't handle non-async requests as they are not handled via callbacks
      return originalOpen.apply(this, arguments)
    }

    // Set the url on the request so we can reference it later
    // Store only primitive types not things like URL objects
    this[localURLProp] = arguments[1]
    return originalOpen.apply(this, arguments)
  }, /*overrideToString=*/false);

  const originalSend = XMLHttpRequest.prototype.send
  XMLHttpRequest.prototype.send = $(function () {
    if (this[localURLProp] === undefined) {
      return originalSend.apply(this, arguments)
    }

    // Extract the URL object by combining it with window.location
    let resourceURL

    try {
      // We do this in a try/catch block to not fail the request in case we can't
      // create a URL
      resourceURL = new URL(this[localURLProp], window.location.href)
    } catch (error) {
      // Ignore this error and proceed like a regular request
      return originalSend.apply(this, arguments)
    }

    // Ask iOS if we need to block this request
    sendMessage(resourceURL).then(blocked => {
      if (blocked) {
        Object.defineProperties(this, {
          readyState: { value: 4 }
        })
        this.dispatchEvent(patchProgressEvent(new ProgressEvent('loadstart')))
        this.dispatchEvent(new Event('readystatechange'))
        this.dispatchEvent(patchProgressEvent(new ProgressEvent('error')))
        this.dispatchEvent(patchProgressEvent(new ProgressEvent('loadend')))
      } else {
        originalSend.apply(this, arguments)
      }
    })
  }, /*overrideToString=*/false);
});
