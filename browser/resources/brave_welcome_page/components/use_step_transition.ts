/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

const stepRenderedEvent = 'welcome-step-rendered'

export function useStepTransition() {
  React.useEffect(() => {
    window.dispatchEvent(new CustomEvent(stepRenderedEvent))
  }, [])
}

export function whenStepRendered() {
  return new Promise<void>((resolve) => {
    window.addEventListener(stepRenderedEvent, () => resolve(), { once: true })
  })
}
