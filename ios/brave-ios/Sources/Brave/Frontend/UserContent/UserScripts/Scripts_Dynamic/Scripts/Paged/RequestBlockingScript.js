// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

"use strict";

window.__firefox__.execute(function($) {
  const messageHandler = '$<message_handler>';
  const blockingCache = new Map();
  const strippedBlockingCache = new Map();
  const sendMessage = $((resourceURL) => {
    let strippedURL = new URL(resourceURL)
    strippedURL.hash = ''
    strippedURL.search = ''

    if (blockingCache.has(resourceURL.href)) {
      return Promise.resolve(blockingCache.get(resourceURL.href)).then(blocked => {
        if (blocked) {
          console.info(
            `Brave prevented frame displaying ${window.location.href} from loading a resource from ${resourceURL.href} (cached)`
          )
        }
        return blocked
      })
    } else if (
      strippedBlockingCache.has(strippedURL.href) 
        && strippedBlockingCache.get(strippedURL.href) === false
    ) {
      // We only check false because if we are supposed to block this request, 
      // content blockers will still kick in and block it. The opposite is not true.
      return Promise.resolve(false)
    }

    return $.postNativeMessage(messageHandler, {
      "securityToken": SECURITY_TOKEN,
      "data": {
        resourceURL: resourceURL.href,
        strippedResourceURL: strippedURL.href,
        sourceURL: window.location.href,
        resourceType: 'xmlhttprequest'
      }
    }).then(response => {
      blockingCache.set(resourceURL.href, response.isBlocked)
      if (response.isBlocked === response.isStrippedURLBlocked) {
        strippedBlockingCache.set(strippedURL.href, response.isStrippedURLBlocked)
      }

      if (response.isBlocked) {
        console.info(
          `Brave prevented frame displaying ${window.location.href} from loading a resource from ${resourceURL.href}`
        )
      }

      return response.isBlocked
    });
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
        this.dispatchEvent(new Event('readystatechange'))
        this.dispatchEvent(new ProgressEvent('error'))
      } else {
        originalSend.apply(this, arguments)
      }
    })
  }, /*overrideToString=*/false);
});
