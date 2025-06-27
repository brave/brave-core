/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

const eventName = 'ntp-top-site-added'

export function dispatchTopSiteAddedEvent() {
  document.dispatchEvent(new CustomEvent(eventName))
}

export function useTopSiteAddedEvent(callback: () => void, deps?: any[]) {
  React.useEffect(() => {
    document.addEventListener(eventName, callback)
    return () => document.removeEventListener(eventName, callback)
  }, deps ?? [])
}
