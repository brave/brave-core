// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Button from '$web-components/button'

import * as S from './style'
import { ButtonText, IconBox } from '../general'
import { AlertCircleIcon, CaratStrongLeftIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../../common/locale'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'

interface Props {
  showContactSupport: React.MouseEventHandler<HTMLButtonElement>
}

function ErrorPanel (props: Props) {
  const dispatch = useDispatch()
  const currentRegion = useSelector(state => state.currentRegion)

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
        <S.PanelHeader>
          <S.BackButton
            type='button'
            onClick={handleShowMainView}
            aria-label='Go to main'
          >
            <i><CaratStrongLeftIcon /></i>
            <span>{getLocale('braveVpnErrorPanelHeader')}</span>
          </S.BackButton>
        </S.PanelHeader>
        <IconBox>
          <AlertCircleIcon color='#84889C' />
        </IconBox>
        <S.ReasonTitle>{getLocale('braveVpnUnableConnectToServer')}</S.ReasonTitle>
        <S.ReasonDesc>
          {getLocale('braveVpnUnableConnectInfo').replace(/\$\d+/g, (match) => matches[match])}
        </S.ReasonDesc>
        <S.ActionArea>
          <Button
            type={'submit'}
            isPrimary
            isCallToAction
            onClick={handleTryAgain}
          >
            {getLocale('braveVpnTryAgain')}
          </Button>
          <ButtonText onClick={handleChooseServer}>
            {getLocale('braveVpnChooseAnotherServer')}
          </ButtonText>
          <ButtonText onClick={props.showContactSupport}>
            {getLocale('braveVpnContactSupport')}
          </ButtonText>
        </S.ActionArea>
      </S.PanelContent>
    </S.Box>
  )
}

export default ErrorPanel
