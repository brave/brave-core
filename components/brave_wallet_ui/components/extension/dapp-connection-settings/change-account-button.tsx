// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

// Components
import {
  CreateAccountIcon //
} from '../../shared/create-account-icon/create-account-icon'
import { LoadingSkeleton } from '../../shared/loading-skeleton/index'

// Queries
import {
  useSelectedAccountQuery //
} from '../../../common/slices/api.slice.extra'
import {
  useGetActiveOriginConnectedAccountIdsQuery,
  useGetDefaultFiatCurrencyQuery,
  useRemoveSitePermissionMutation,
  useRequestSitePermissionMutation,
  useSetSelectedAccountMutation
} from '../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../constants/types'

// Styled Components
import {
  ActiveIndicator,
  DescriptionText,
  NameText
} from './dapp-connection-settings.style'
import {
  Row,
  Column,
  VerticalSpace,
  HorizontalSpace //
} from '../../shared/style'

interface Props {
  account: BraveWallet.AccountInfo
  getAccountsFiatValue: (account: BraveWallet.AccountInfo) => Amount
}

export const ChangeAccountButton = (props: Props) => {
  const { account, getAccountsFiatValue } = props

  // Queries
  const { data: selectedAccount } = useSelectedAccountQuery()
  const isActive =
    account.accountId.uniqueKey === selectedAccount?.accountId.uniqueKey
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: connectedAccounts = [] } =
    useGetActiveOriginConnectedAccountIdsQuery()

  // mutations
  const [setSelectedAccount] = useSetSelectedAccountMutation()
  const [removeSitePermission] = useRemoveSitePermissionMutation()
  const [requestSitePermission] = useRequestSitePermissionMutation()

  // Constants
  const selectedCoin = selectedAccount?.accountId.coin

  // Memos
  const hasPermission = React.useMemo((): boolean => {
    return connectedAccounts.some(
      (accountId) => accountId.uniqueKey === account.accountId.uniqueKey
    )
  }, [connectedAccounts, account.accountId.uniqueKey])

  const buttonText = React.useMemo((): string => {
    const connectText =
      selectedCoin === BraveWallet.CoinType.SOL
        ? getLocale('braveWalletSitePermissionsTrust')
        : getLocale('braveWalletAddAccountConnect')
    const disconnectText =
      selectedCoin === BraveWallet.CoinType.SOL
        ? getLocale('braveWalletSitePermissionsRevoke')
        : getLocale('braveWalletSitePermissionsDisconnect')
    return hasPermission
      ? isActive
        ? disconnectText
        : getLocale('braveWalletSitePermissionsSwitch')
      : connectText
  }, [selectedCoin, hasPermission, isActive])

  const accountFiatValue = React.useMemo(() => {
    return getAccountsFiatValue(account)
  }, [getAccountsFiatValue, account])

  // Methods
  const onClickConnect = React.useCallback(async () => {
    await requestSitePermission(account.accountId).unwrap()

    if (selectedCoin !== BraveWallet.CoinType.SOL) {
      setSelectedAccount(account.accountId)
    }
  }, [
    requestSitePermission,
    account.accountId,
    selectedCoin,
    setSelectedAccount
  ])

  const onClickDisconnect = React.useCallback(async () => {
    await removeSitePermission(account.accountId).unwrap()

    if (
      connectedAccounts.length !== 0 &&
      selectedCoin !== BraveWallet.CoinType.SOL
    ) {
      setSelectedAccount(account.accountId)
    }
  }, [
    removeSitePermission,
    account.accountId,
    connectedAccounts.length,
    selectedCoin,
    setSelectedAccount
  ])

  const onClickSwitchAccount = React.useCallback(() => {
    setSelectedAccount(account.accountId)
  }, [account.accountId, setSelectedAccount])

  const onClickConnectDisconnectOrSwitch = React.useCallback(() => {
    return hasPermission
      ? isActive
        ? onClickDisconnect()
        : onClickSwitchAccount()
      : onClickConnect()
  }, [
    hasPermission,
    isActive,
    onClickDisconnect,
    onClickConnect,
    onClickSwitchAccount
  ])

  return (
    <Row
      justifyContent='space-between'
      padding='8px 0px'
    >
      <Row width='unset'>
        <CreateAccountIcon
          size='medium'
          account={account}
          marginRight={16}
        />
        <Column alignItems='flex-start'>
          <NameText
            textSize='14px'
            isBold={true}
          >
            {account.name}
          </NameText>
          <Row
            width='unset'
            justifyContent='flex-start'
          >
            <DescriptionText
              textSize='12px'
              isBold={false}
            >
              {reduceAddress(account.accountId.address)}
            </DescriptionText>
            {isActive && (
              <>
                <HorizontalSpace space='8px' />
                <ActiveIndicator>
                  {getLocale('braveWalletActive')}
                </ActiveIndicator>
              </>
            )}
          </Row>
          {accountFiatValue.isUndefined() ? (
            <>
              <VerticalSpace space='3px' />
              <LoadingSkeleton
                width={60}
                height={12}
              />
              <VerticalSpace space='3px' />
            </>
          ) : (
            <DescriptionText
              textSize='12px'
              isBold={false}
            >
              {accountFiatValue.formatAsFiat(defaultFiatCurrency)}
            </DescriptionText>
          )}
        </Column>
      </Row>
      <Row width='unset'>
        <Button
          onClick={onClickConnectDisconnectOrSwitch}
          kind='outline'
          size='small'
        >
          {buttonText}
        </Button>
      </Row>
    </Row>
  )
}
