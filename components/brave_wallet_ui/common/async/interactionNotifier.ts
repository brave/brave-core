// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

class InteractionNotifier {
  private intervalId: ReturnType<typeof setInterval> | undefined
  userHasInteracted = false

  private handleInteraction = () => {
    this.userHasInteracted = true
  }

  private handleVisibilityChange = () => {
    if (document.visibilityState === 'visible') {
      this.userHasInteracted = true
    }
  }

  beginWatchingForInteraction(
    timeMs: number,
    isWalletLocked: boolean,
    onInteractionInterval: () => unknown
  ) {
    if (!isWalletLocked && !this.intervalId) {
      this.intervalId = setInterval(() => {
        if (this.userHasInteracted) {
          this.userHasInteracted = false
          onInteractionInterval()
        }
      }, timeMs)
      window.addEventListener('mousemove', this.handleInteraction)
      window.addEventListener('keydown', this.handleInteraction)
      window.addEventListener('focus', this.handleInteraction, true)
      window.addEventListener('scroll', this.handleInteraction, true)
      document.addEventListener('visibilitychange', this.handleVisibilityChange)
    }
  }

  stopWatchingForInteraction() {
    if (this.intervalId) {
      clearTimeout(this.intervalId)
      this.intervalId = undefined
      window.removeEventListener('mousemove', this.handleInteraction)
      window.removeEventListener('keydown', this.handleInteraction)
      window.removeEventListener('focus', this.handleInteraction, true)
      window.removeEventListener('scroll', this.handleInteraction, true)
      document.removeEventListener(
        'visibilitychange',
        this.handleVisibilityChange
      )
    }
  }
}

export const interactionNotifier = new InteractionNotifier()
