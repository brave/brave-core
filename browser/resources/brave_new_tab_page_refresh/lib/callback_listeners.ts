/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

type Router<T> = {
  [P in keyof T]: {
    addListener: (listener: any) => number
  }
} & {
  removeListener: (callbackId: number) => void
}

export function addCallbackListeners<T>(
  router: Router<T>,
  listeners: Partial<T>
) {
  const callbackIds = Object.entries(listeners).map(([key, value]) => {
    return router[key as keyof T].addListener(value)
  })
  return () => {
    callbackIds.forEach(id => router.removeListener(id))
  }
}
