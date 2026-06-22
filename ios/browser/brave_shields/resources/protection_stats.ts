// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWebKitMessage } from '//ios/web/public/js_messaging/resources/utils.js'

const messageHandlerName = 'ProtectionStatsMessageHandler'

interface BlockedResource {
  resourceURL: string
  resourceType: string
}

let sendInfo: BlockedResource[] = []
let sendInfoTimeout: ReturnType<typeof setTimeout> | null = null

function sendMessage(
  urlString: string | null | undefined,
  resourceType: string,
): void {
  // String is empty, null, undefined, ...
  if (!urlString) {
    return
  }

  let resourceURL: URL
  try {
    resourceURL = new URL(urlString, document.location.href)

    // First party urls or invalid URLs are not blocked
    if (document.location.host === resourceURL.host) {
      return
    }
  } catch (error) {
    console.error(error)
    return
  }

  sendInfo.push({ resourceURL: resourceURL.href, resourceType })

  if (sendInfoTimeout) {
    return
  }

  // Send the URLs in batches every 500ms to avoid perf issues from calling
  // js-to-native too frequently.
  sendInfoTimeout = setTimeout(() => {
    sendInfoTimeout = null
    if (sendInfo.length === 0) {
      return
    }

    sendWebKitMessage(messageHandlerName, sendInfo)
    sendInfo = []
  }, 500)
}

function onLoad(): void {
  // Send back the sources of every script and image in the DOM back to the
  // host application.
  Array.prototype.slice
    .apply(document.scripts)
    .forEach((el: HTMLScriptElement) => sendMessage(el.src, 'script'))
  Array.prototype.slice
    .apply(document.images)
    .forEach((el: HTMLImageElement) => sendMessage(el.src, 'image'))
  Array.prototype.slice
    .apply(document.getElementsByTagName('subdocument'))
    .forEach((el: any) => sendMessage(el.src, 'subdocument'))
}

window.addEventListener('load', onLoad, false)

// A property used to track the URL of a synchronous XMLHttpRequest.
const localURLProp = Symbol('url')
// A property used to track whether an error handler has already been attached
// to an XMLHttpRequest or Image instance.
const localErrorHandlerProp = Symbol('tpErrorHandler')

// -------------------------------------------------
// Send XHR requests URLs to the host application
// -------------------------------------------------
const originalXHROpen = XMLHttpRequest.prototype.open
const originalXHRSend = XMLHttpRequest.prototype.send

XMLHttpRequest.prototype.open = function (this: any, ...args: any[]): void {
  const url = args[1]
  const isAsync = args[2]

  // Blocked async XMLHttpRequest are handled via RequestBlocking.js. We only
  // handle sync requests.
  if (isAsync === undefined || isAsync) {
    return originalXHROpen.apply(this, args as any)
  }

  this[localURLProp] = url
  return originalXHROpen.apply(this, args as any)
}

XMLHttpRequest.prototype.send = function (this: any, ...args: any[]): void {
  const url = this[localURLProp]
  if (!url) {
    return originalXHRSend.apply(this, args as any)
  }

  // Only attach the `error` event listener once for this `XMLHttpRequest`
  // instance.
  if (!this[localErrorHandlerProp]) {
    // If this `XMLHttpRequest` instance fails to load, we can assume it has
    // been blocked.
    this[localErrorHandlerProp] = () => {
      sendMessage(url, 'xmlhttprequest')
    }

    this.addEventListener('error', this[localErrorHandlerProp])
  }
  return originalXHRSend.apply(this, args as any)
}

// -------------------------------------------------
// Send `fetch()` request URLs to the host application
// -------------------------------------------------
const originalFetch = window.fetch

window.fetch = function (...args: any[]) {
  const input = args[0]
  if (typeof input === 'string') {
    sendMessage(input, 'xmlhttprequest')
  } else if (input instanceof Request) {
    sendMessage(input.url, 'xmlhttprequest')
  }

  return originalFetch.apply(window, args as any)
}

// -------------------------------------------------
// Detect when new sources get set on Image and send them to the host
// application
// -------------------------------------------------
const originalImageSrc = Object.getOwnPropertyDescriptor(Image.prototype, 'src')

if (originalImageSrc && originalImageSrc.get && originalImageSrc.set) {
  delete (Image.prototype as any).src

  Object.defineProperty(Image.prototype, 'src', {
    get: function (): string {
      return originalImageSrc.get!.call(this)
    },

    set: function (this: any, value: string): void {
      // Only attach the `error` event listener once for this Image instance.
      if (!this[localErrorHandlerProp]) {
        // If this `Image` instance fails to load, we can assume it has been
        // blocked.
        this[localErrorHandlerProp] = () => {
          sendMessage(this.src, 'image')
        }

        this.addEventListener('error', this[localErrorHandlerProp])
      }

      originalImageSrc.set!.call(this, value)
    },
    enumerable: true,
    configurable: true,
  })
}

// -------------------------------------------------
// Listen to when new <script> elements get added to the DOM and send the
// source to the host application
// -------------------------------------------------
const mutationObserver = new MutationObserver((mutations: MutationRecord[]) => {
  mutations.forEach((mutation: MutationRecord) => {
    mutation.addedNodes.forEach((node: Node) => {
      const element = node as HTMLElement

      // `<script src="*">` elements.
      if (element.tagName === 'SCRIPT' && (element as HTMLScriptElement).src) {
        sendMessage((element as HTMLScriptElement).src, 'script')
        return
      }

      if (element.tagName === 'IMG' && (element as HTMLImageElement).src) {
        sendMessage((element as HTMLImageElement).src, 'image')
        return
      }

      // `<iframe src="*">` elements where [src] is not "about:blank".
      if (element.tagName === 'IFRAME' && (element as HTMLIFrameElement).src) {
        if ((element as HTMLIFrameElement).src === 'about:blank') {
          return
        }

        sendMessage((element as HTMLIFrameElement).src, 'subdocument')
      }
    })
  })
})

mutationObserver.observe(document.documentElement, {
  childList: true,
  subtree: true,
})
