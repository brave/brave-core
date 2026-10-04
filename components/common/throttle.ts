// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

export default function throttle (fn: Function, bufferTimeMs: number, callAtEnd: boolean = false) {
  let hasCalled = false
  let pendingCallArgs: any[] | null = null
  return function (...callArgs: any[]) {
    if (!hasCalled) {
      pendingCallArgs = null
      hasCalled = true
      fn(...callArgs)
      // Do not allow function to be called for x seconds
      setTimeout(function () {
        hasCalled = false
        if (pendingCallArgs) {
          fn(...callArgs)
          pendingCallArgs = null
        }
      }, bufferTimeMs)
    } else {
      // Save args so we can call after interval
      pendingCallArgs = callArgs
    }
  }
}

export function requestAnimationFrameThrottle (fn: Function) {
  let hasPendingCall = false
  return function (...callArgs: any[]) {
    if (!hasPendingCall) {
      hasPendingCall = true
      window.requestAnimationFrame(() => {
        hasPendingCall = false
        fn(...callArgs)
      })
    } else {
      // We are waiting for animation frame,
      // skip this extra call
    }
  }
}
