// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'
import {
  useGetPendingSignMessageErrorsQuery,
  useProcessSignMessageErrorMutation,
} from '../../../common/slices/api.slice'

// Components
import CreateSiteOrigin from '../../shared/create-site-origin/index'

// Style
import {
  ErrorWrapper,
  ErrorTitle,
  ErrorBox,
  ErrorDescriptionText,
  WarningIconWrapper,
  WarningIcon,
  OriginErrorTitle,
  OriginErrorText,
  LaunchButton,
  LaunchIcon,
} from './sign_in_with_ethereum_error.style'
import { Row, Column } from '../../shared/style'

export const SignInWithEthereumError = () => {
  // Queries
  const { data: signMessageErrorData } = useGetPendingSignMessageErrorsQuery()

  // Mutations
  const [processSignMessageError] = useProcessSignMessageErrorMutation()

  // Computed
  const errorData = signMessageErrorData?.[0]

  const message = errorData?.localizedErrMsg ?? ''
  const address = message.substring(
    message.indexOf('(') + 1,
    message.indexOf(')'),
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
            'tabs.create failed: ' + chrome.runtime.lastError.message,
          )
        }
      },
    )
  }

  const onClickClose = async () => {
    if (errorData?.id) {
      await processSignMessageError(errorData.id).unwrap()
    }
  }

  return (
    <ErrorWrapper
      fullWidth={true}
      fullHeight={true}
      justifyContent='space-between'
    >
      <Row padding='18px 0px'>
        <ErrorTitle textColor='primary'>
          {getLocale('braveWalletSecurityRiskDetected')}
        </ErrorTitle>
      </Row>
      <ErrorBox
        fullWidth={true}
        justifyContent='space-between'
        padding='0px 42px'
        gap='34px'
      >
        <Column>
          <WarningIconWrapper
            padding='12px'
            margin='0px 0px 8px 0px'
          >
            <WarningIcon />
          </WarningIconWrapper>
          <OriginErrorTitle textColor='primary'>
            {errorData?.originInfo.eTldPlusOne ?? ''}
          </OriginErrorTitle>
          <OriginErrorText textColor='error'>
            <CreateSiteOrigin
              originSpec={errorData?.originInfo.originSpec ?? ''}
              eTldPlusOne={errorData?.originInfo.eTldPlusOne ?? ''}
            />
          </OriginErrorText>
        </Column>

        <ErrorDescriptionText textColor='primary'>
          {errorMessage}
          {errorData?.chainId && (
            <LaunchButton onClick={onClickViewOnChainList}>
              <LaunchIcon />
            </LaunchButton>
          )}
        </ErrorDescriptionText>
      </ErrorBox>
      <Row padding='16px'>
        <Button onClick={onClickClose}>
          {getLocale('braveWalletButtonClose')}
        </Button>
      </Row>
    </ErrorWrapper>
  )
}
