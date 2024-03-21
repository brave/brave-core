// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '$web-common/locale'

import * as S from './style'

import Button from '$web-components/button'

import { WelcomeBrowserProxyImpl } from '../../api/welcome_browser_proxy'

import cubeImg from '../../assets/brave-search-cube.svg'
import DataContext from '../../state/context'
import { useViewTypeTransition } from '../../state/hooks'

function HelpWDP () {
  const { viewType, setViewType } = React.useContext(DataContext)

  const { forward } = useViewTypeTransition(viewType)

  const handleAccept = () => {
    WelcomeBrowserProxyImpl.getInstance().enableWebDiscovery()
    setViewType(forward)
  }
  const handleReject = () => {
    setViewType(forward)
  }

  return (
    <S.MainBox>
      <div className='view-header-box'>
        <img
          className='view-logo-box'
          src={cubeImg}
        />
        <h1 className='view-title'>{getLocale('braveWelcomeHelpWDPTitle')}</h1>
        <h2 className='view-subtitle'>
          {getLocale('braveWelcomeHelpWDPSubtitle')}
        </h2>
      </div>
      <S.BodyBox>
        {getLocale('braveWelcomeHelpWDPDescription')}
        <a
          href='https://support.brave.com/hc/en-us/articles/4409406835469-What-is-the-Web-Discovery-Project'
          target='_blank'
        >
          {getLocale('braveWelcomeHelpWDPLearnMore')}
        </a>
      </S.BodyBox>
      <S.ActionBox>
        <Button
          isPrimary={true}
          scale='jumbo'
          onClick={handleAccept}
        >
          {getLocale('braveWelcomeHelpWDPAccept')}
        </Button>
        <Button
          isTertiary={true}
          scale='jumbo'
          onClick={handleReject}
        >
          {getLocale('braveWelcomeHelpWDPReject')}
        </Button>
      </S.ActionBox>
    </S.MainBox>
  )
}

export default HelpWDP
