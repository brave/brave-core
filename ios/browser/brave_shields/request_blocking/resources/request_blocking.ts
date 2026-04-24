// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWebKitMessageWithReply} from '//ios/web/public/js_messaging/resources/utils.js';

const blockingCache = new Map<string, boolean>();
const sendMessage = (resourceURL: URL) => {
  // when location.href does not match origin, we include origin in console
  const originDisplay = window.location.href !== window.origin ? ` (${window.origin})` : ``;
  if (blockingCache.has(resourceURL.href)) {
    return Promise.resolve(blockingCache.get(resourceURL.href)!).then(blocked => {
      if (blocked) {
        console.info(`Brave prevented frame displaying ${window.location.href}${originDisplay} from loading a resource from ${resourceURL.href} (cached)`)
      }
      return blocked
    })
  }

  return sendWebKitMessageWithReply('RequestBlockingMessageHandler', { resourceURL: resourceURL.href, resourceType: 'xmlhttprequest' }).then((blocked: boolean) => {
    blockingCache.set(resourceURL.href, blocked)

    if (blocked) {
      console.info(`Brave prevented frame displaying ${window.location.href}${originDisplay} from loading a resource from ${resourceURL.href}`)
    }

    return blocked
  });
};

const patchProgressEvent = (progressEvent: ProgressEvent) =>{
  Object.setPrototypeOf(progressEvent, (window as any).XMLHttpRequestProgressEvent.prototype)
  Object.defineProperties(progressEvent, {
    position: { value: 0 },
    totalSize: { value: 0 }
  })
  return progressEvent
};

const { fetch: originalFetch } = window
window.fetch = function(...args: Parameters<typeof fetch>) {
  const [resource] = args

  // Extract the url
  let urlString
  if (typeof resource === 'string') {
    // Simple send where resource is a string
    urlString = resource
  } else if (resource instanceof Request) {
    // Assume it's a Request
    urlString = resource.url
  } else {
    return originalFetch.apply(this, args)
  }

  const url = new URL(urlString, window.location.href)
  return sendMessage(url).then((blocked: boolean) => {
    if (blocked) {
      return Promise.reject(new TypeError('Load failed'))
    } else {
      return originalFetch.apply(this, args)
    }
  })
};

const xhrURLMap = new WeakMap<XMLHttpRequest, string>()
const originalOpen = XMLHttpRequest.prototype.open
XMLHttpRequest.prototype.open = function(_method: string, url: string | URL, isAsync?: boolean, _username?: string | null, _password?: string | null) {
  // Check if we're async
  if (isAsync !== undefined && !isAsync) {
    // We can't handle non-async requests as they are not handled via callbacks
    return originalOpen.apply(this, arguments as any)
  }

  xhrURLMap.set(this, url instanceof URL ? url.href : url)
  return originalOpen.apply(this, arguments as any)
};

const originalSend = XMLHttpRequest.prototype.send
XMLHttpRequest.prototype.send = function(...args: Parameters<typeof XMLHttpRequest.prototype.send>) {
  if (!xhrURLMap.has(this)) {
    return originalSend.apply(this, args)
  }

  // Extract the URL object by combining it with window.location
  let resourceURL

  try {
    // We do this in a try/catch block to not fail the request in case we can't
    // create a URL
    resourceURL = new URL(xhrURLMap.get(this)!, window.location.href)
  } catch (error) {
    // Ignore this error and proceed like a regular request
    return originalSend.apply(this, args)
  }

  // Ask iOS if we need to block this request
  sendMessage(resourceURL).then((blocked: boolean) => {
    if (blocked) {
      Object.defineProperties(this, {
        readyState: { value: 4 }
      })
      this.dispatchEvent(patchProgressEvent(new ProgressEvent('loadstart')))
      this.dispatchEvent(new Event('readystatechange'))
      this.dispatchEvent(patchProgressEvent(new ProgressEvent('error')))
      this.dispatchEvent(patchProgressEvent(new ProgressEvent('loadend')))
    } else {
      originalSend.apply(this, args)
    }
  })
};
