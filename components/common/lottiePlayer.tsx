// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

interface LottiePlayerProps {
  animationUrl: string
  autoPlay: boolean
  singleLoop: boolean
  onInitialized?: () => void
  onPlaying?: () => void
  onPaused?: () => void
  onStopped?: () => void
  onComplete?: () => void
}

function LottiePlayer (props: LottiePlayerProps) {
  const setupRender = React.useCallback((elem: HTMLElement | null) => {
    if (elem) {
      const lottieEl = document.createElement('cr-lottie')
      lottieEl.addEventListener('cr-lottie-initialized', () => props.onInitialized?.())
      lottieEl.addEventListener('cr-lottie-playing', () => props.onPlaying?.())
      lottieEl.addEventListener('cr-lottie-paused', () => props.onPaused?.())
      lottieEl.addEventListener('cr-lottie-stopped', () => props.onStopped?.())
      lottieEl.addEventListener('cr-lottie-complete', () => props.onComplete?.())
      // @ts-expect-error
      lottieEl.animationUrl = props.animationUrl
      // @ts-expect-error
      lottieEl.autoplay = props.autoPlay
      // @ts-expect-error
      lottieEl.singleLoop = props.singleLoop
      elem.appendChild(lottieEl)
    }
  }, [])

  return (
    <div ref={setupRender} />
  )
}

export default LottiePlayer
