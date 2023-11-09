// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import Button from '@brave/leo/react/button'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'

// Selectors
import { useUnsafePanelSelector } from '../../../common/hooks/use-safe-selector'
import { PanelSelectors } from '../../../panel/selectors'

// Components
import CreateSiteOrigin from '../../shared/create-site-origin/index'

import {
  StyledWrapper,
  Title,
  ErrorBox,
  ErrorDescriptionText,
  WarningIcon,
  OriginErrorText,
  LaunchButton,
  LaunchIcon
} from './sign_in_with_ethereum.style'
import { Row, Column, VerticalDivider, VerticalSpace } from '../../shared/style'
import { PanelActions } from '../../../panel/actions'

export const SignInWithEthereumError = () => {
  // Redux
  const dispatch = useDispatch()

  // Selectors
  const signMessageErrorData = useUnsafePanelSelector(
    PanelSelectors.signMessageErrorData
  )

  // Computed
  const errorData =
    signMessageErrorData.length === 0 ? undefined : signMessageErrorData[0]

  const message = errorData?.localizedErrMsg ?? ''
  const address = message.substring(
    message.indexOf('(') + 1,
    message.indexOf(')')
  )

  const errorMessage =
    !errorData?.localizedErrMsg && !errorData?.type
      ? ''
      : errorData.type === BraveWallet.SignMessageErrorType.kAccountMismatched
      ? message.replace(address, reduceAddress(address))
      : errorData.localizedErrMsg

  // Methods
  const onClickViewOnChainList = () => {
    if (!errorData?.chainId) {
      return
    }
    chrome.tabs.create(
      { url: `https://chainlist.org/chain/${errorData.chainId}` },
      () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message
          )
        }
      }
    )
  }

  const onClickClose = () => {
    if (!errorData?.id) {
      return
    }
    dispatch(PanelActions.signMessageErrorProcessed(errorData.id))
  }

  return (
    <StyledWrapper>
      <>
        <Row padding='16px 16px 11px 16px'>
          <Title
            isBold={true}
            textSize='18px'
          >
            {getLocale('braveWalletSecurityRiskDetected')}
          </Title>
        </Row>
        <VerticalDivider />
      </>
      <Column
        fullWidth={true}
        fullHeight={true}
        justifyContent='space-between'
        padding='16px'
      >
        <ErrorBox>
          <WarningIcon />
          <ErrorDescriptionText
            textSize='16px'
            isBold={true}
          >
            {errorData?.originInfo.eTldPlusOne ?? ''}
          </ErrorDescriptionText>
          <VerticalSpace space='8px' />
          <OriginErrorText textSize='12px'>
            <CreateSiteOrigin
              originSpec={errorData?.originInfo.originSpec ?? ''}
              eTldPlusOne={errorData?.originInfo.eTldPlusOne ?? ''}
            />
          </OriginErrorText>

          <ErrorDescriptionText
            textSize='14px'
            isBold={false}
          >
            {errorMessage}
            {errorData?.chainId && (
              <LaunchButton onClick={onClickViewOnChainList}>
                <LaunchIcon />
              </LaunchButton>
            )}
          </ErrorDescriptionText>
        </ErrorBox>
        <Row>
          <Button onClick={onClickClose}>
            {getLocale('braveWalletButtonClose')}
          </Button>
        </Row>
      </Column>
    </StyledWrapper>
  )
}
