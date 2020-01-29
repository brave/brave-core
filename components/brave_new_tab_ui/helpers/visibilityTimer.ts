// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * Waits until the viewport has been in view for a certain number of seconds
 * continuously before calling a provided function.
 */
export default class VisibilityTimer {
  private timerId?: number = undefined
  private viewTimeMs: number = 0
  private onTimerExpired: Function

  constructor (onTimerExpired: Function, viewTimeMs: number) {
    this.onTimerExpired = onTimerExpired
    this.viewTimeMs = viewTimeMs
  }

  startTracking () {
    document.addEventListener(
      'visiblitychange',
      this.handleVisibility
    )
    this.handleVisibility()
  }

  stopTracking () {
    this.resetTimer()
    // Stop tracking visibility.
    document.removeEventListener(
      'visiblitychange',
      this.handleVisibility
    )
  }

  private handleVisibility = () => {
    if (document.visibilityState === 'visible') {
      if (!this.timerId) {
        // Start the timer again
        this.startWaiting()
      }
    } else {
      // Stop the timer. It will start again when the page is next visible.
      this.resetTimer()
    }
  }

  private startWaiting () {
    if (this.timerId) {
      return
    }
    this.timerId = setTimeout(() => {
      // If we made it here, then we have received enough uninterupted time
      // and we can call the provided function.
      this.stopTracking()
      this.onTimerExpired()
    }, this.viewTimeMs)
  }

  private resetTimer () {
    clearTimeout(this.timerId)
    this.timerId = undefined
  }
}
