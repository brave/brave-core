// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { useHistory } from 'react-router'

// utils
import { getLocale, splitStringForTag } from '../../../../../common/locale'

// types
import { PageState, WalletRoutes } from '../../../../constants/types'

// style
import {
  StyledWrapper,
  Title,
  Description,
  PageIcon,
  RestoreButton,
  Divider,
  ImportButton,
  SettingsButton,
  ImportButtonText,
  MetaMaskIcon,
  CryptoWalletsAlertBox,
  CryptoWalletsAlertTitle,
  CryptoWalletsAlertDescription
} from './style'

// components
import { NavButton } from '../../../extension'

export const OnboardingWelcome = () => {
  // routing
  const history = useHistory()

  // redux
  const {
    isCryptoWalletsInitialized,
    isMetaMaskInitialized
  } = useSelector(({ page }: { page: PageState }) => page)

  // methods
  const onRestore = React.useCallback(() => {
    history.push(WalletRoutes.Restore)
  }, [])

  const onClickImportMetaMask = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingImportMetaMask)
  }, [])

  const onClickSettings = React.useCallback(() => {
    chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [])

  const onSetup = React.useCallback(() => {
    if (isCryptoWalletsInitialized) {
      history.push(WalletRoutes.OnboardingImportCryptoWallets)
      return
    }
    history.push(WalletRoutes.OnboardingCreatePassword)
  }, [isCryptoWalletsInitialized])

  // computed
  const { beforeTag, duringTag, afterTag } = splitStringForTag(
    getLocale('braveWalletCryptoWalletsDescriptionTwo')
  )

  return (
    <StyledWrapper>

      <PageIcon />

      <Title>{getLocale('braveWalletWelcomeTitle')}</Title>
      <Description>{getLocale('braveWalletWelcomeDescription')}</Description>

      <NavButton buttonType='primary' text={getLocale('braveWalletWelcomeButton')} onSubmit={onSetup} />

      <RestoreButton onClick={onRestore}>{getLocale('braveWalletWelcomeRestoreButton')}</RestoreButton>

      {isMetaMaskInitialized &&
        <>
          <Divider />
          <ImportButton onClick={onClickImportMetaMask}>
            <MetaMaskIcon />
            <ImportButtonText>{getLocale('braveWalletImportTitle').replace('$1', getLocale('braveWalletImportMetaMaskTitle'))}</ImportButtonText>
          </ImportButton>
        </>
      }

      {isCryptoWalletsInitialized &&
        <CryptoWalletsAlertBox>
          <CryptoWalletsAlertTitle>{getLocale('braveWalletCryptoWalletsDetected')}</CryptoWalletsAlertTitle>
          <CryptoWalletsAlertDescription>{getLocale('braveWalletCryptoWalletsDescriptionOne')}</CryptoWalletsAlertDescription>
          <CryptoWalletsAlertDescription>
            {beforeTag}
            <SettingsButton onClick={onClickSettings}>{duringTag}</SettingsButton>
            {afterTag}
          </CryptoWalletsAlertDescription>
        </CryptoWalletsAlertBox>
      }
    </StyledWrapper>
  )
}

export default OnboardingWelcome
