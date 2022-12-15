// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

class WebAnimationPlayer {
  animations: Animation[]
  timingOption: OptionalEffectTiming

  constructor (timingOption = { duration: 1000, fill: 'both' as FillMode, easing: 'ease-in-out' }) {
    this.animations = []
    this.timingOption = timingOption
  }

  to (element: Element | null, keyFrame: PropertyIndexedKeyframes, keyFrameOption?: OptionalEffectTiming) {
    const kFOptions = { ...this.timingOption, ...keyFrameOption }
    const kF = new KeyframeEffect(element, keyFrame, kFOptions)
    const animation = new Animation(kF)
    this.animations.push(animation)
    return this
  }

  play () {
    this.animations.forEach((animation) => {
      animation.persist() // persist states of animated values before playing because we might need to go back to the state when reversing it
      animation.playbackRate = 1 // Keep the playback rate consistent even after reversing it
      animation.play()
    })
  }

  reverse () {
    this.animations.forEach((animation) => {
      animation.playbackRate = -1
      animation.play()
    })
  }

  finish () {
    this.animations.forEach((animation) => {
      animation.finish()
    })
  }

  cancel () {
    this.animations.forEach((animation) => {
      animation.cancel()
    })
  }
}

export default WebAnimationPlayer
