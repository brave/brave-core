// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWebKitMessageWithReply } from '//ios/web/public/js_messaging/resources/utils.js'

interface BlockingResult {
  blocked: boolean
  redirect?: string
  rewrittenURL?: string
}

// Returns the URL the request should be sent to instead, if any. Redirects take
// precedence over blocking as they replace the resource rather than failing it.
const replacementURL = (result: BlockingResult): string | undefined => {
  if (result.redirect) {
    return result.redirect
  }
  if (!result.blocked && result.rewrittenURL) {
    return result.rewrittenURL
  }
  return undefined
}

const isBlocked = (result: BlockingResult) => result.blocked && !result.redirect

const blockingCache = new Map<string, BlockingResult>()
const sendMessage = (resourceURL: URL): Promise<BlockingResult> => {
  // when location.href does not match origin, we include origin in console
  const originDisplay =
    window.location.href !== window.origin ? ` (${window.origin})` : ``
  const log = (result: BlockingResult, suffix: string) => {
    if (result.redirect) {
      console.info(
        `Brave redirected a resource from ${resourceURL.href} for frame displaying ${window.location.href}${originDisplay}${suffix}`,
      )
    } else if (result.blocked) {
      console.info(
        `Brave prevented frame displaying ${window.location.href}${originDisplay} from loading a resource from ${resourceURL.href}${suffix}`,
      )
    }
  }

  const cached = blockingCache.get(resourceURL.href)
  if (cached) {
    log(cached, ' (cached)')
    return Promise.resolve(cached)
  }

  return sendWebKitMessageWithReply('RequestBlockingMessageHandler', {
    resourceURL: resourceURL.href,
    resourceType: 'xmlhttprequest',
  }).then((reply: BlockingResult | undefined) => {
    const result: BlockingResult = reply ?? { blocked: false }
    blockingCache.set(resourceURL.href, result)
    log(result, '')
    return result
  })
}

const patchProgressEvent = (progressEvent: ProgressEvent) => {
  Object.setPrototypeOf(
    progressEvent,
    (window as any).XMLHttpRequestProgressEvent.prototype,
  )
  Object.defineProperties(progressEvent, {
    position: { value: 0 },
    totalSize: { value: 0 },
  })
  return progressEvent
}

const { fetch: originalFetch } = window
window.fetch = function (...args: Parameters<typeof fetch>) {
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
  return sendMessage(url).then((result: BlockingResult) => {
    if (isBlocked(result)) {
      return Promise.reject(new TypeError('Load failed'))
    }
    const replacement = replacementURL(result)
    if (replacement) {
      const [, init] = args
      const request =
        resource instanceof Request
          ? new Request(replacement, resource)
          : replacement
      return originalFetch.call(this, request, init)
    }
    return originalFetch.apply(this, args)
  })
}

type XHROpenArgs = [
  string,
  string | URL,
  boolean?,
  (string | null)?,
  (string | null)?,
]
const xhrURLMap = new WeakMap<XMLHttpRequest, string>()
const xhrOpenArgsMap = new WeakMap<XMLHttpRequest, XHROpenArgs>()
const xhrHeadersMap = new WeakMap<XMLHttpRequest, [string, string][]>()
const originalOpen = XMLHttpRequest.prototype.open
XMLHttpRequest.prototype.open = function (...args: XHROpenArgs) {
  const [_method, url, isAsync] = args
  // Check if we're async
  if (isAsync !== undefined && !isAsync) {
    // We can't handle non-async requests as they are not handled via callbacks
    return originalOpen.apply(this, args as any)
  }

  xhrURLMap.set(this, url instanceof URL ? url.href : url)
  xhrOpenArgsMap.set(this, args)
  xhrHeadersMap.set(this, [])
  return originalOpen.apply(this, args as any)
}

// Headers are recorded so they can be replayed if the request has to be
// re-opened with a redirected or rewritten URL.
const originalSetRequestHeader = XMLHttpRequest.prototype.setRequestHeader
XMLHttpRequest.prototype.setRequestHeader = function (
  name: string,
  value: string,
) {
  xhrHeadersMap.get(this)?.push([name, value])
  return originalSetRequestHeader.call(this, name, value)
}

const originalSend = XMLHttpRequest.prototype.send
XMLHttpRequest.prototype.send = function (
  ...args: Parameters<typeof XMLHttpRequest.prototype.send>
) {
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
  sendMessage(resourceURL).then((result: BlockingResult) => {
    const replacement = replacementURL(result)
    const openArgs = xhrOpenArgsMap.get(this)
    if (replacement && openArgs) {
      const [method, , isAsync, username, password] = openArgs
      originalOpen.call(
        this,
        method,
        replacement,
        isAsync ?? true,
        username,
        password,
      )
      for (const [name, value] of xhrHeadersMap.get(this) ?? []) {
        originalSetRequestHeader.call(this, name, value)
      }
      originalSend.apply(this, args)
    } else if (isBlocked(result)) {
      Object.defineProperties(this, {
        readyState: { value: 4 },
      })
      this.dispatchEvent(patchProgressEvent(new ProgressEvent('loadstart')))
      this.dispatchEvent(new Event('readystatechange'))
      this.dispatchEvent(patchProgressEvent(new ProgressEvent('error')))
      this.dispatchEvent(patchProgressEvent(new ProgressEvent('loadend')))
    } else {
      originalSend.apply(this, args)
    }
  })
}
