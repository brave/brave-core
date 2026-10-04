/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

const stepRenderedEvent = 'welcome-step-rendered'

// Triggers a step rendered event, used for view transitions to indicate when
// the next step view has finished rendering.
export function useStepTransition() {
  React.useEffect(() => {
    window.dispatchEvent(new CustomEvent(stepRenderedEvent))
  }, [])
}

// Returns a promise that resolves when the next step component is rendered.
export function whenStepRendered() {
  return new Promise<void>((resolve) => {
    window.addEventListener(stepRenderedEvent, () => resolve(), { once: true })
  })
}
