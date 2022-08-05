// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

window.braveBlockRequests = (args) => {
  'use strict'
  const messageHandler = webkit.messageHandlers[args.handlerName]
  const securityToken = args.securityToken
  const sendMessage = (resourceURL) => {
    return messageHandler.postMessage({
      securityToken: securityToken,
      data: {
        resourceURL: resourceURL.href,
        sourceURL: window.location.href,
        resourceType: 'xmlhttprequest'
      }
    })
  }
  
  const { fetch: originalFetch } = window
  window.fetch = function () {
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
        return new Promise(function (resolve, reject) {
          reject(new TypeError('Load failed'))
        })
      } else {
        return originalFetch.apply(this, arguments)
      }
    })
  }
  
  const originalOpen = XMLHttpRequest.prototype.open
  XMLHttpRequest.prototype.open = function () {
    // Check if we're async
    const isAsync = arguments[2]
    if (isAsync !== undefined && !isAsync) {
      // We can't handle non-async requests as they are not handled via callbacks
      return originalOpen.apply(this, arguments)
    }

    // Extract the url
    const urlString = arguments[1]
    this._resourceURL = new URL(urlString, window.location.href)
    return originalOpen.apply(this, arguments)
  }
  
  const originalSend = XMLHttpRequest.prototype.send
  XMLHttpRequest.prototype.send = function () {
    if (this._resourceURL === undefined) {
      return originalSend.apply(this, arguments)
    }
    
    sendMessage(this._resourceURL).then(blocked => {
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
  }
}

// Invoke window.braveBlockRequests then delete the function
