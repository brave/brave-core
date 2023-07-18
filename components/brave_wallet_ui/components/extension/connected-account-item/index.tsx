// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// Actions
import { WalletActions } from '../../../common/actions'

// Types
import { BraveWallet } from '../../../constants/types'

// Hooks
import {
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'

// Utils
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'
import { useSetSelectedAccountMutation } from '../../../common/slices/api.slice'
import { useSelectedAccountQuery } from '../../../common/slices/api.slice.extra'
import { findAccountByAccountId } from '../../../utils/account-utils'
import { WalletSelectors } from '../../../common/selectors'

// Hooks
import { useAccountOrb } from '../../../common/hooks/use-orb'

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
  account: BraveWallet.AccountInfo
}

const SitePermissionAccountItem = (props: Props) => {
  const {
    account
  } = props

  const dispatch = useDispatch()
  const connectedAccounts = useUnsafeWalletSelector(WalletSelectors.connectedAccounts)

  // api
  const { data: selectedAccount } = useSelectedAccountQuery()
  const [setSelectedAccount] = useSetSelectedAccountMutation()
  const selectedCoin = selectedAccount?.accountId.coin

  // memos
  const orb = useAccountOrb(account)

  const isActive = account.accountId.uniqueKey === selectedAccount?.accountId.uniqueKey

  const hasPermission = React.useMemo((): boolean => {
    return !!findAccountByAccountId(connectedAccounts, account.accountId)
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
    dispatch(WalletActions.addSitePermission({ accountId: account.accountId }))
    if (selectedCoin !== BraveWallet.CoinType.SOL) {
      setSelectedAccount(account.accountId)
    }
  }, [account, selectedCoin])

  const onClickDisconnect = React.useCallback(() => {
    dispatch(WalletActions.removeSitePermission({ accountId: account.accountId }))
    if (connectedAccounts.length !== 0 && selectedCoin !== BraveWallet.CoinType.SOL) {
      setSelectedAccount(account.accountId)
    }
  }, [connectedAccounts, account, selectedCoin])

  const onClickSwitchAccount = React.useCallback(() => {
    setSelectedAccount(account.accountId)
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
