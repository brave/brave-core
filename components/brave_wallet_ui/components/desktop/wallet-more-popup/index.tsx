// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// Types
import { BraveWallet, WalletState } from '../../../constants/types'

// actions
import { WalletActions } from '../../../common/actions'

// utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  SettingsIcon,
  LockIcon,
  ExplorerIcon,
  BackupIcon,
  ConnectedSitesIcon,
  HelpCenterIcon
} from './style'

export interface Props {
  onClickSetting?: () => void
  onClickViewOnBlockExplorer?: () => void
  onClickBackup?: () => void
}

const WalletMorePopup = (props: Props) => {
  const {
    onClickSetting,
    onClickViewOnBlockExplorer,
    onClickBackup
  } = props

  // redux
  const dispatch = useDispatch()
  const selectedNetwork = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetwork)

  // methods
  const lockWallet = React.useCallback(() => {
    dispatch(WalletActions.lockWallet())
  }, [])

  const onClickConnectedSites = React.useCallback(() => {
    const route = selectedNetwork.coin === BraveWallet.CoinType.ETH ? 'ethereum' : 'solana'
    chrome.tabs.create({ url: `brave://settings/content/${route}` }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [selectedNetwork])

  const onClickHelpCenter = React.useCallback(() => {
    chrome.tabs.create({ url: 'https://support.brave.com/hc/en-us/articles/4415497656461-Brave-Wallet-FAQ' }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [])

  return (
    <StyledWrapper>

      <PopupButton onClick={lockWallet}>
        <LockIcon />
        <PopupButtonText>{getLocale('braveWalletWalletPopupLock')}</PopupButtonText>
      </PopupButton>

      {onClickBackup &&
        <PopupButton onClick={onClickBackup}>
          <BackupIcon />
          <PopupButtonText>{getLocale('braveWalletBackupButton')}</PopupButtonText>
        </PopupButton>
      }

      {selectedNetwork.coin !== BraveWallet.CoinType.FIL &&
        <PopupButton onClick={onClickConnectedSites}>
          <ConnectedSitesIcon />
          <PopupButtonText>{getLocale('braveWalletWalletPopupConnectedSites')}</PopupButtonText>
        </PopupButton>
      }

      {onClickSetting &&
        <PopupButton onClick={onClickSetting}>
          <SettingsIcon />
          <PopupButtonText>{getLocale('braveWalletWalletPopupSettings')}</PopupButtonText>
        </PopupButton>
      }

      {onClickViewOnBlockExplorer &&
        <PopupButton onClick={onClickViewOnBlockExplorer}>
          <ExplorerIcon />
          <PopupButtonText>{getLocale('braveWalletTransactionExplorer')}</PopupButtonText>
        </PopupButton>
      }

      <PopupButton onClick={onClickHelpCenter}>
        <HelpCenterIcon />
        <PopupButtonText>{getLocale('braveWalletHelpCenter')}</PopupButtonText>
      </PopupButton>
    </StyledWrapper>
  )
}

export default WalletMorePopup
