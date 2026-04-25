// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import DataContext from '../../state/context'
import { shouldPlayAnimations } from '../../state/hooks'

function ImportInProgress () {
  const { scenes } = React.useContext(DataContext)
  const ref = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    scenes?.s2.play()
  }, [])

  React.useEffect(() => {
    if (!ref.current) return
    if (!shouldPlayAnimations) return

    const els = ref.current.querySelectorAll('span')
    const animations: Animation[] = []

    els.forEach((el, i) => {
      const animation = el.animate({ opacity: 1 }, { duration: 500, fill: 'forwards', easing: 'ease-in-out', delay: i * 500 })
      animations.push(animation)
    })

    const onLastAnimationFinished = () => animations.forEach(animation => animation.play())
    animations[els.length - 1].addEventListener('finish', onLastAnimationFinished)

    return () => {
      animations[els.length - 1].removeEventListener('finish', onLastAnimationFinished)
      animations.forEach(animation => animation.cancel())
    }
  }, [])

  return (
    <S.Box>
      <div className="view-title">Importing</div>
      <div className="dots" ref={ref}>
        <span></span>
        <span></span>
        <span></span>
      </div>
    </S.Box>
  )
}

export default ImportInProgress
