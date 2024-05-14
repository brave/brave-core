// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as S from './style'

import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import Button from '$web-components/button'

import { WelcomeBrowserProxyImpl, DefaultBrowserBrowserProxyImpl, P3APhase } from '../../api/welcome_browser_proxy'
import WebAnimationPlayer from '../../api/web_animation_player'

import DataContext from '../../state/context'
import { shouldPlayAnimations, useViewTypeTransition } from '../../state/hooks'

import braveLogoUrl from '../../assets/brave_logo_3d@2x.webp'

function Welcome () {
  const { viewType, setViewType, scenes } = React.useContext(DataContext)
  const { forward } = useViewTypeTransition(viewType)

  const ref = React.useRef<HTMLDivElement>(null)

  const goForward = () => setViewType(forward)

  const handleSetAsDefaultBrowser = () => {
    DefaultBrowserBrowserProxyImpl.getInstance().setAsDefaultBrowser()
    goForward()
    WelcomeBrowserProxyImpl.getInstance().recordP3A(P3APhase.Import)
    scenes?.s1.play()
  }

  const handleSkip = () => {
    WelcomeBrowserProxyImpl.getInstance().recordP3A(P3APhase.Import)
    goForward()
    scenes?.s1.play()
  }

  React.useEffect(() => {
    if (!ref.current) return
    if (!shouldPlayAnimations) return

    const logoBoxEl = ref.current.querySelector('.view-logo-box')
    const backdropEl = ref.current.querySelector('.view-backdrop')
    const contentEl = ref.current.querySelector('.view-content')

    const s1 = new WebAnimationPlayer()

    s1.to(logoBoxEl, {
        transform: 'translateY(-20px)',
        filter: 'drop-shadow(7px 2px 5px rgba(14, 1, 41, 0.2)) drop-shadow(14px 3px 10px rgba(32, 5, 89, 0.3)) drop-shadow(20px 3px 15px rgba(37, 7, 87, 0.2))  drop-shadow(25px 5px 30px rgba(25, 3, 73, 0.1)) drop-shadow(50px 4px 50px rgba(19, 3, 40, 0.1))'
        }, { fill: 'forwards', easing: 'ease-out' })
      .to(backdropEl, { scale: 1, opacity: 1 }, { duration: 250, delay: 200, easing: 'ease-out' })
      .to(contentEl, { transform: 'translateY(0px)', opacity: 1 }, { duration: 250, delay: 200, easing: 'ease-out' })

    s1.play()

    return () => {
      s1.finish()
      s1.cancel()
    }
  }, [])

  return (
    <S.Box ref={shouldPlayAnimations ? ref : null}>
      <div className="view-logo-box">
        <img src={braveLogoUrl} />
      </div>
      <div className={classnames({ 'view-content': true, 'initial': shouldPlayAnimations })}>
        <div className="view-header-box">
          <div className="view-details">
            <h1 className="view-title">{getLocale('braveWelcomeTitle')}</h1>
            <p className="view-desc">{getLocale('braveWelcomeDesc')}</p>
          </div>
        </div>
        <S.ActionBox>
          <Button
            isPrimary={true}
            onClick={handleSetAsDefaultBrowser}
            scale="jumbo"
          >
            {getLocale('braveWelcomeSetDefaultButtonLabel')}
          </Button>
          <Button
            isTertiary={true}
            onClick={handleSkip}
            scale="jumbo"
          >
            {getLocale('braveWelcomeSkipButtonLabel')}
          </Button>
        </S.ActionBox>
      </div>
      <div className={classnames({ 'view-backdrop': true, 'initial': shouldPlayAnimations })} />
    </S.Box>
  )
}

export default Welcome
