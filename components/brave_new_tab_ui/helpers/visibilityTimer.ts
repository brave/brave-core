// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * Waits until the viewport (and an optional element) has been in view for a certain number of seconds
 * continuously before calling a provided function.
 */
export default class VisibilityTimer {
  private element?: HTMLElement
  private timerId?: number = undefined
  private isIntersecting: boolean
  private viewTimeMs: number = 0
  private onTimerExpired: Function
  private intersectionObserver?: IntersectionObserver

  constructor (onTimerExpired: Function, viewTimeMs: number, elementToObserve?: HTMLElement) {
    this.element = elementToObserve
    this.onTimerExpired = onTimerExpired
    this.viewTimeMs = viewTimeMs
    this.isIntersecting = elementToObserve ? false : true
    if (elementToObserve) {
      this.intersectionObserver = new IntersectionObserver(entries => {
        this.isIntersecting = entries.some(entry => entry.isIntersecting)
        this.handleVisibility()
      }, {
        threshold: 0.5
      })
    }
  }

  startTracking () {
    document.addEventListener(
      'visiblitychange',
      this.handleVisibility
    )
    if (this.intersectionObserver && this.element) {
      this.intersectionObserver.observe(this.element)
    }
    this.handleVisibility()
  }

  stopTracking () {
    this.resetTimer()
    // Stop tracking visibility.
    document.removeEventListener(
      'visiblitychange',
      this.handleVisibility
    )
    if (this.intersectionObserver && this.element) {
      this.intersectionObserver.unobserve(this.element)
    }
  }

  private handleVisibility = () => {
    if (document.visibilityState === 'visible' && this.isIntersecting) {
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
    this.timerId = window.setTimeout(() => {
      // If we made it here, then we have received enough uninterupted time
      // and we can call the provided function.
      this.stopTracking()
      this.onTimerExpired()
    }, this.viewTimeMs)
  }

  private resetTimer () {
    window.clearTimeout(this.timerId)
    this.timerId = undefined
  }
}
