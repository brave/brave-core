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

  // Debounce multiple state changes in the same task causing multiple
  // events to be dispatched.
  private dispatchDebouncedStateChange = debounce(() => {
    this.eventTarget.dispatchEvent(
      new Event('statechange')
    )
  }, 0)

  public setPartialState(partialState: Partial<T>) {
    console.log('setPartialState', partialState)
    this.state = { ...this.state, ...partialState }
    this.dispatchDebouncedStateChange()
  }

  public addStateChangeListener(callback: (event: Event) => void) {
    this.eventTarget.addEventListener('statechange', callback)
  }

  public removeStateChangeListener(callback: (event: Event) => void) {
    this.eventTarget.removeEventListener('statechange', callback)
  }
}
