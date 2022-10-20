// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import Button from '$web-components/button'
import DataContext from '../../state/context'
import { ViewType } from '../../state/component_types'
import { DefaultBrowserBrowserProxyImpl } from '../../api/default_browser_proxy'
import { getLocale } from '$web-common/locale'

function Welcome () {
  const { setViewType } = React.useContext(DataContext)

  const handleSetAsDefaultBrowser = () => {
    DefaultBrowserBrowserProxyImpl.getInstance().setAsDefaultBrowser()
    setViewType(ViewType.SelectBrowser)
  }

  const handleSkip = () => {
    setViewType(ViewType.SelectBrowser)
  }

  return (
    <S.MainBox>
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
    </S.MainBox>
  )
}

export default Welcome
