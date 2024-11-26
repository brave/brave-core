// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import { PanelHeader } from '../select-region-list'
import * as Actions from '../../state/actions'
import { useSelector, useDispatch } from '../../state/hooks'
import { getLocale } from '$web-common/locale'

interface Props {
  showContactSupport: () => void
}

function ErrorPanel(props: Props) {
  const dispatch = useDispatch()
  const currentRegion = useSelector((state) => state.currentRegion)

  const handleShowMainView = () => {
    dispatch(Actions.resetConnectionState())
  }

  const handleTryAgain = () => {
    dispatch(Actions.connect())
  }

  const handleChooseServer = () => {
    dispatch(Actions.toggleRegionSelector(true))
  }

  const matches = {
    $1: getLocale('braveVpn'),
    $2: currentRegion?.namePretty || ''
  }

  return (
    <S.Box>
      <S.PanelContent>
        <PanelHeader
          title={getLocale('braveVpn')}
          buttonAriaLabel={getLocale('braveVpnErrorPanelBackButtonAriaLabel')}
          onClick={handleShowMainView}

        />
        <S.TopContent>
          <S.StyledAlert
            type='error'
            mode='full'
            hideIcon
          >
            <div slot='title'>{getLocale('braveVpnUnableConnectToServer')}</div>
            {getLocale('braveVpnUnableConnectInfo').replace(
              /\$\d+/g,
              (match) => matches[match]
            )}
          </S.StyledAlert>
          <S.StyledActionButton
            slot='actions'
            kind='filled'
            onClick={handleTryAgain}
          >
            {getLocale('braveVpnTryAgain')}
          </S.StyledActionButton>
          <S.StyledActionButton
            slot='actions'
            kind='plain'
            onClick={handleChooseServer}
          >
            {getLocale('braveVpnChooseAnotherServer')}
          </S.StyledActionButton>
          <S.StyledActionButton
            slot='actions'
            kind='plain'
            onClick={props.showContactSupport}
          >
            {getLocale('braveVpnContactSupport')}
          </S.StyledActionButton>
        </S.TopContent>
      </S.PanelContent>
    </S.Box>
  )
}

export default ErrorPanel
