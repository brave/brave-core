// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import DataContext from '../../state/context'
import { useShouldPlayAnimations } from '../../state/hooks'
import { DefaultBrowserBrowserProxyImpl } from '../../api/default_browser_proxy'
import { ViewType } from '../../state/component_types'
import { BravePrivacyBrowserProxyImpl } from '../../api/privacy_data_browser'
import Button from '$web-components/button'
import { getLocale } from '$web-common/locale'
import braveLogoUrl from '../../assets/brave_logo_3d@2x.png'

function Welcome () {
  const { setViewType, scenes } = React.useContext(DataContext)
  const ref = React.useRef<HTMLDivElement>(null)
  const shouldPlayAnimations = useShouldPlayAnimations()

  const handleSetAsDefaultBrowser = () => {
    BravePrivacyBrowserProxyImpl.getInstance().recordP3A({ currentScreen: ViewType.Default, isFinished: false, isSkipped: false })
    DefaultBrowserBrowserProxyImpl.getInstance().setAsDefaultBrowser()
    setViewType(ViewType.SelectBrowser)
    scenes?.s1.play()
  }

  const handleSkip = () => {
    BravePrivacyBrowserProxyImpl.getInstance().recordP3A({ currentScreen: ViewType.Default, isFinished: false, isSkipped: true })
    setViewType(ViewType.SelectBrowser)
    scenes?.s1.play()
  }

  React.useEffect(() => {
    if (!ref.current) return
    if (!shouldPlayAnimations) return

    const logoAnim: Animation = ref.current.animate({
      transform: 'translateY(-20px)',
      filter: 'drop-shadow(7px 2px 5px rgba(14, 1, 41, 0.2)) drop-shadow(14px 3px 10px rgba(32, 5, 89, 0.3)) drop-shadow(20px 3px 15px rgba(37, 7, 87, 0.2))  drop-shadow(25px 5px 30px rgba(25, 3, 73, 0.1)) drop-shadow(50px 4px 50px rgba(19, 3, 40, 0.1))'
    }, { duration: 1000, fill: 'forwards', easing: 'ease-out' })

    return () => {
      logoAnim.finish()
      logoAnim.cancel()
    }
  }, [])

  return (
    <S.Box>
      <div ref={ref} className="brave-logo-box">
        <img src={braveLogoUrl} />
      </div>
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
    </S.Box>
  )
}

export default Welcome
