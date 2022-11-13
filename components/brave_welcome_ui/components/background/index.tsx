// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import DataContext from '../../state/context'
import WebAnimationPlayer from '../../api/web_animation_player'

import Stars01 from '../svg/stars01'
import Stars02 from '../svg/stars02'
import Stars03 from '../svg/stars03'
import Stars04 from '../svg/stars04'

interface BackgroundProps {
  children?: JSX.Element
  static: boolean
}

function AnimatedBackground (props: { children?: JSX.Element }) {
  const ref = React.useRef<HTMLDivElement>(null)
  const { setScenes } = React.useContext(DataContext)

  React.useEffect(() => {
    if (!ref.current) return

    const s1 = new WebAnimationPlayer()
    const s2 = new WebAnimationPlayer()

    const hill01 = ref.current.querySelector('.hills01')
    const hill02 = ref.current.querySelector('.hills02')
    const hills03 = ref.current.querySelector('.hills03')

    const stars01 = ref.current.querySelector('.stars01')
    const stars02 = ref.current.querySelector('.stars02')
    const stars03 = ref.current.querySelector('.stars03')
    const stars04 = ref.current.querySelector('.stars04')
    const pyramid = ref.current.querySelector('.pyramid')

    s1.to(hill01, { transform: 'translateX(-650px) scale(2.5)', filter: 'blur(3px)' })
      .to(hill02, { transform: 'scale(1.5)' })
      .to(stars01, { transform: 'scale(2.5)' })
      .to(stars02, { transform: 'scale(3.5)', filter: 'blur(3px)' })
      .to(stars03, { transform: 'scale(2.5)' })
      .to(stars04, { opacity: 1, transform: 'scale(1)' })

    s2.to(hill01, { transform: 'translateX(-120%)' })
      .to(hill02, { transform: 'translateX(-500px) scale(4.0)', filter: 'blur(3px)' })
      .to(hills03, { transform: 'scale(2.5)' })
      .to(pyramid, { transform: 'translateX(0px)', backgroundSize: '40%', filter: 'blur(3px)' })
      .to(stars02, { transform: 'scale(5.0)' })
      .to(stars03, { transform: 'scale(3.5)', filter: 'blur(3px)' })
      .to(stars04, { transform: 'scale(1.5)' })

    setScenes({ s1, s2 })
  }, [])

  return (
    <S.Box ref={ref}>
      <div className="stars-container">
        <Stars01 />
        <Stars02 />
        <Stars03 />
        <Stars04 />
      </div>
      <div className="content-box">
        {props.children}
      </div>
      <div className="sky" />
      <div className="hills-container">
        <div className="hills-base hills03"></div>
        <div className="hills-base hills02"></div>
        <div className="hills-base hills01"></div>
        <div className="pyramid"></div>
      </div>
    </S.Box>
  )
}

function Background (props: BackgroundProps) {
  if (!props.static) {
    return (<AnimatedBackground>
        {props.children}
      </AnimatedBackground>
    )
  }

  return (
    <S.StaticBackground>
      {props.children}
    </S.StaticBackground>
  )
}

export default Background
