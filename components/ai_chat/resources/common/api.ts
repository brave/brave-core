// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { debounce } from '$web-common/debounce'

export default abstract class API<T> {
  public state: T
  private eventTarget = new EventTarget()

  constructor(defaultState: T) {
    this.state = {...defaultState}
  }

  private dispatchDebouncedStateChange = debounce(() => {
    console.debug('dispatching uistatechange event', {...this.state})
    this.eventTarget.dispatchEvent(
      new Event('uistatechange')
    )
  }, 0)

  public setPartialState(partialState: Partial<T>) {
    this.state = { ...this.state, ...partialState }
    this.dispatchDebouncedStateChange()
  }

  public addStateChangeListener(callback: (event: Event) => void) {
    this.eventTarget.addEventListener('uistatechange', callback)
  }

  public removeStateChangeListener(callback: (event: Event) => void) {
    this.eventTarget.removeEventListener('uistatechange', callback)
  }
}
