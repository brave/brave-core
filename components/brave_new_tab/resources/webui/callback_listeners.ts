/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

type Router<T> = {
  [P in keyof T]: {
    addListener: (listener: any) => void
    removeListener: (listener: any) => void
  }
}

export function addCallbackListeners<T>(
  router: Router<T>,
  listeners: Partial<T>
) {
  for (const [key, value] of Object.entries(listeners)) {
    router[key as keyof T].addListener(value)
  }
  return () => {
    for (const [key, value] of Object.entries(listeners)) {
      router[key as keyof T].removeListener(value)
    }
  }
}
