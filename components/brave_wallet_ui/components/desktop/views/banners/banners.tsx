// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'

// Page API Proxy
import getWalletPageApiProxy from '../../../../page/wallet_page_api_proxy'

// Utils
import { getLocale } from '../../../../../common/locale'
import { openWalletSettings } from '../../../../utils/routes-utils'

// Selectors
import { useSafeUISelector } from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'

// Types
import { WalletRoutes, BraveWallet } from '../../../../constants/types'

// Components
import WalletBanner from '../../wallet-banner'
import {
  useGetDefaultEthereumWalletQuery,
  useGetIsWalletBackedUpQuery,
  useGetDefaultSolanaWalletQuery,
  useGetIsMetaMaskInstalledQuery,
} from '../../../../common/slices/api.slice'

export const Banners = () => {
  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isAndroid = useSafeUISelector(UISelectors.isAndroid)

  // Queries
  const {
    data: isMetaMaskInstalled,
    isLoading: isCheckingInstalledExtensions,
  } = useGetIsMetaMaskInstalledQuery()
  const {
    data: defaultEthereumWallet,
    isLoading: isLoadingDefaultEthereumWallet,
  } = useGetDefaultEthereumWalletQuery()
  const { data: defaultSolanaWallet, isLoading: isLoadingDefaultSolanaWallet } =
    useGetDefaultSolanaWalletQuery()
  const {
    data: isWalletBackedUp = false,
    isLoading: isCheckingWalletBackupStatus,
  } = useGetIsWalletBackedUpQuery()

  // State
  const [isBackupWarningDismissed, setDismissBackupWarning] =
    React.useState<boolean>(isWalletBackedUp)
  const [isDefaultWalletBannerDismissed, setDismissDefaultWalletBanner] =
    React.useState<boolean>(false)

  // routing
  const history = useHistory()

  const isCheckingWallets =
    isCheckingInstalledExtensions
    || isLoadingDefaultEthereumWallet
    || isLoadingDefaultSolanaWallet

  const showBanner =
    !isCheckingWallets
    && (defaultEthereumWallet !== BraveWallet.DefaultWallet.BraveWallet
      || defaultSolanaWallet !== BraveWallet.DefaultWallet.BraveWallet)
    && (defaultEthereumWallet
      !== BraveWallet.DefaultWallet.BraveWalletPreferExtension
      || defaultSolanaWallet
        !== BraveWallet.DefaultWallet.BraveWalletPreferExtension
      || (defaultEthereumWallet
        === BraveWallet.DefaultWallet.BraveWalletPreferExtension
        && isMetaMaskInstalled))
    && !isDefaultWalletBannerDismissed

  // Methods
  const onShowBackup = React.useCallback(() => {
    if (isAndroid) {
      getWalletPageApiProxy().pageHandler.showWalletBackupUI()
      return
    }

    if (isPanel) {
      chrome.tabs.create(
        {
          url: `chrome://wallet${WalletRoutes.Backup}`,
        },
        () => {
          if (chrome.runtime.lastError) {
            console.error(
              'tabs.create failed: ' + chrome.runtime.lastError.message,
            )
          }
        },
      )
      return
    }
    history.push(WalletRoutes.Backup)
  }, [isAndroid, isPanel, history])

  return (
    <>
      {showBanner && (
        <WalletBanner
          onDismiss={() => {
            setDismissDefaultWalletBanner(true)
          }}
          onClick={openWalletSettings}
          bannerType='warning'
          buttonText={getLocale('braveWalletWalletPopupSettings')}
          description={getLocale('braveWalletDefaultWalletBanner')}
        />
      )}
      {!isCheckingWalletBackupStatus
        && !isWalletBackedUp
        && !isBackupWarningDismissed && (
          <WalletBanner
            onDismiss={() => {
              setDismissBackupWarning(true)
            }}
            onClick={onShowBackup}
            bannerType='error'
            buttonText={getLocale('braveWalletBackupButton')}
            description={getLocale('braveWalletBackupWarningText')}
          />
        )}
    </>
  )
}
