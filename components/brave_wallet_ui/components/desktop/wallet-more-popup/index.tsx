// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

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
  ConnectedSitesIcon
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

  // methods
  const lockWallet = React.useCallback(() => {
    dispatch(WalletActions.lockWallet())
  }, [])

  const onClickConnectedSites = React.useCallback(() => {
    chrome.tabs.create({ url: 'brave://settings/content/ethereum' }, () => {
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

      <PopupButton onClick={onClickConnectedSites}>
        <ConnectedSitesIcon />
        <PopupButtonText>{getLocale('braveWalletWalletPopupConnectedSites')}</PopupButtonText>
      </PopupButton>

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
    </StyledWrapper>
  )
}

export default WalletMorePopup
