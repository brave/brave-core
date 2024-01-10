// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.execute(function($) {
  //Cookie should neither be saved nor accessed (local or session) when user has blocked all cookies.
  const cookie = Object.getOwnPropertyDescriptor(Document.prototype, 'cookie') || Object.getOwnPropertyDescriptor(HTMLDocument.prototype, 'cookie')

  if (cookie && cookie.configurable) {
    Object.defineProperty(document, 'cookie', {
      get: $(function() {
        // Not returning null here as some websites don't have a check for cookie returning null, and may not behave properly
        return "";
      }),
      set: $(function(val) {
        console.error("Access denied for 'cookie'")
        return;
      })
    });
  }

  //Access to localStorage should be denied when user has blocked all Cookies.
  if (Object.getOwnPropertyDescriptor(window, 'localStorage')) {
    Object.defineProperty(window, 'localStorage', {
      get: $(function() {
        console.error("Access denied for 'localStorage'")
        return null;
      }),
    });
  }

  //Access to sessionStorage should be denied when user has blocked all Cookies.
  if (Object.getOwnPropertyDescriptor(window, 'sessionStorage')) {
    Object.defineProperty(window, 'sessionStorage', {
      get: $(function() {
        console.error("Access denied for 'sessionStorage'")
        return null;
      })
    });
  }

  (() => {
    // Access to caches should be denied when user has blocked all Cookies.
    const makeFailingPromiseFunction = $(function() {
      return $(function() {
        return Promise.reject(
          new DOMException('An attempt was made to break through the security policy of the user agent.')
        )
      });
    });

    // We need to check that window.caches is defined as this API was only added in iOS 15
    // Later on we can probably remove this check.
    if (window.caches !== undefined) {
      window.CacheStorage.prototype.match = makeFailingPromiseFunction()
      window.CacheStorage.prototype.has = makeFailingPromiseFunction()
      window.CacheStorage.prototype.delete = makeFailingPromiseFunction()
      window.CacheStorage.prototype.open = makeFailingPromiseFunction()
      window.CacheStorage.prototype.keys = makeFailingPromiseFunction()
    }
  })()
});
