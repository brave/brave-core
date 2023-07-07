// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// Actions
import { WalletActions } from '../../../common/actions'

// Types
import { BraveWallet, WalletAccountType } from '../../../constants/types'

// Hooks
import { useSelectedCoinQuery } from '../../../common/slices/api.slice'
import {
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'

// Utils
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { create } from 'ethereum-blockies'
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'
import { WalletSelectors } from '../../../common/selectors'

// Styled Components
import {
  StyledWrapper,
  AccountAddressText,
  AccountNameText,
  NameAndAddressColumn,
  AccountCircle,
  LeftSide,
  PrimaryButton,
  RightSide
} from './style'

export interface Props {
  account: WalletAccountType
}

const SitePermissionAccountItem = (props: Props) => {
  const {
    account
  } = props

  const dispatch = useDispatch()
  const selectedAccount = useUnsafeWalletSelector(WalletSelectors.selectedAccount)
  const connectedAccounts = useUnsafeWalletSelector(WalletSelectors.connectedAccounts)
  const activeOrigin = useUnsafeWalletSelector(WalletSelectors.activeOrigin)

  // api
  const { selectedCoin } = useSelectedCoinQuery()

  // memos
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  const isActive = React.useMemo((): boolean => {
    return account.address.toLowerCase() === selectedAccount?.address.toLowerCase()
  }, [selectedAccount?.address, account.address])

  const hasPermission = React.useMemo((): boolean => {
    return connectedAccounts.some(a => a.address.toLowerCase() === account.address.toLowerCase())
  }, [connectedAccounts, account])

  const buttonText = React.useMemo((): string => {
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      return hasPermission
        ? getLocale('braveWalletSitePermissionsRevoke')
        : getLocale('braveWalletSitePermissionsTrust')
    }
    return hasPermission
      ? isActive
        ? getLocale('braveWalletSitePermissionsDisconnect')
        : getLocale('braveWalletSitePermissionsSwitch')
      : getLocale('braveWalletAddAccountConnect')
  }, [selectedCoin, hasPermission, isActive])

  // methods
  const onClickConnect = React.useCallback(() => {
    dispatch(WalletActions.addSitePermission({ accountId: account.accountId, origin: activeOrigin.origin }))
    if (selectedCoin !== BraveWallet.CoinType.SOL) {
      dispatch(WalletActions.selectAccount(account.accountId))
    }
  }, [activeOrigin, account, selectedCoin])

  const onClickDisconnect = React.useCallback(() => {
    dispatch(WalletActions.removeSitePermission({ accountId: account.accountId, origin: activeOrigin.origin }))
    if (connectedAccounts.length !== 0 && selectedCoin !== BraveWallet.CoinType.SOL) {
      dispatch(WalletActions.selectAccount(connectedAccounts[0].accountId))
    }
  }, [connectedAccounts, activeOrigin, account, selectedCoin])

  const onClickSwitchAccount = React.useCallback(() => {
    dispatch(WalletActions.selectAccount(account.accountId))
  }, [account])

  const onClickConnectDisconnectOrSwitch = React.useCallback(() => {
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      return hasPermission
        ? onClickDisconnect()
        : onClickConnect()
    }
    return hasPermission
      ? isActive
        ? onClickDisconnect()
        : onClickSwitchAccount()
      : onClickConnect()
  }, [selectedCoin, hasPermission, isActive, onClickDisconnect, onClickConnect, onClickSwitchAccount])

  return (
    <StyledWrapper>
      <LeftSide>
        <AccountCircle orb={orb} />
        <NameAndAddressColumn>
          <AccountNameText>{reduceAccountDisplayName(account.name, 22)}</AccountNameText>
          <AccountAddressText>
            {reduceAddress(account.address)}
          </AccountAddressText>
        </NameAndAddressColumn>
      </LeftSide>
      <RightSide>
        <PrimaryButton
          onClick={onClickConnectDisconnectOrSwitch}
        >
          {buttonText}
        </PrimaryButton>
      </RightSide>
    </StyledWrapper>
  )
}

export default SitePermissionAccountItem
