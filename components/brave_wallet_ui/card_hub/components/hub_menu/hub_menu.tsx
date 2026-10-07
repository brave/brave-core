// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Constants
import { LOCAL_STORAGE_KEYS } from '$wallet/common/constants/local-storage-keys'

// Utils
import { getLocale } from '$web-common/locale'
import { useSyncedLocalStorage } from '$wallet/common/hooks/use_local_storage'
import { useLockWalletMutation } from '$wallet/common/slices/api.slice'

// Styles
import { ButtonMenu } from './hub_menu.style'
import { FabButton } from '../../card_hub.style'

interface Props {
  onOpenSettings: () => void
}

export const HubMenu = (props: Props) => {
  const { onOpenSettings } = props

  // Local Storage
  const [hidePortfolioBalances, setHidePortfolioBalances] =
    useSyncedLocalStorage(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES, false)

  // Mutations
  const [lockWallet] = useLockWalletMutation()

  // Methods
  const onToggleHideBalances = React.useCallback(() => {
    setHidePortfolioBalances((prev) => !prev)
  }, [setHidePortfolioBalances])

  const onLockWallet = React.useCallback(async () => {
    await lockWallet()
  }, [lockWallet])

  return (
    <ButtonMenu placement='bottom-end'>
      <FabButton slot='anchor-content'>
        <Icon name='more-vertical' />
      </FabButton>
      <leo-menu-item onClick={onToggleHideBalances}>
        <Icon name={hidePortfolioBalances ? 'eye-on' : 'eye-off'} />
        {hidePortfolioBalances
          ? getLocale(S.BRAVE_WALLET_SHOW_BALANCES)
          : getLocale(S.BRAVE_WALLET_HIDE_BALANCES)}
      </leo-menu-item>
      <leo-menu-item onClick={onOpenSettings}>
        <Icon name='settings' />
        {getLocale(S.BRAVE_WALLET_WALLET_POPUP_SETTINGS)}
      </leo-menu-item>
      <leo-menu-item onClick={onLockWallet}>
        <Icon name='lock' />
        {getLocale(S.BRAVE_WALLET_WALLET_POPUP_LOCK)}
      </leo-menu-item>
    </ButtonMenu>
  )
}
