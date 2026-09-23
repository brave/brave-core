// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

function enable(): void {
  // Cookies should neither be saved nor accessed (local or session) when the
  // user has blocked all cookies.
  const cookie =
    Object.getOwnPropertyDescriptor(Document.prototype, 'cookie')
    || Object.getOwnPropertyDescriptor(HTMLDocument.prototype, 'cookie')

  if (cookie && cookie.configurable) {
    Object.defineProperty(document, 'cookie', {
      get: function (): string {
        // Not returning null here as some websites don't have a check for
        // cookie returning null, and may not behave properly.
        return ''
      },
      set: function (_val: string): void {
        console.error("Access denied for 'cookie'")
      },
    })
  }

  // Access to localStorage should be denied when the user has blocked all
  // cookies.
  if (Object.getOwnPropertyDescriptor(window, 'localStorage')) {
    Object.defineProperty(window, 'localStorage', {
      get: function (): Storage | null {
        console.error("Access denied for 'localStorage'")
        return null
      },
    })
  }

  // Access to sessionStorage should be denied when the user has blocked all
  // cookies.
  if (Object.getOwnPropertyDescriptor(window, 'sessionStorage')) {
    Object.defineProperty(window, 'sessionStorage', {
      get: function (): Storage | null {
        console.error("Access denied for 'sessionStorage'")
        return null
      },
    })
  }

  // Access to caches should be denied when the user has blocked all cookies.
  const makeFailingPromiseFunction = function () {
    return function (): Promise<never> {
      return Promise.reject(
        new DOMException(
          'An attempt was made to break through the security policy of the '
            + 'user agent.',
        ),
      )
    }
  }

  if (window.caches !== undefined) {
    window.CacheStorage.prototype.match = makeFailingPromiseFunction()
    window.CacheStorage.prototype.has = makeFailingPromiseFunction()
    window.CacheStorage.prototype.delete = makeFailingPromiseFunction()
    window.CacheStorage.prototype.open = makeFailingPromiseFunction()
    window.CacheStorage.prototype.keys = makeFailingPromiseFunction()
  }
}

if ((window as any).gCrWebPlaceholderBlockAllCookiesEnabled) {
  enable()
}
