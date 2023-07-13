// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// redux
import { useDispatch } from 'react-redux'

// actions
import { AccountsTabActions } from '../../../page/reducers/accounts-tab-reducer'

// utils
import { reduceAddress } from '../../../utils/reduce-address'
import {
  getAccountTypeDescription
} from '../../../utils/account-utils'
import {
  getBalance
} from '../../../utils/balance-utils'
import {
  computeFiatAmount
} from '../../../utils/pricing-utils'
import {
  getPriceIdForToken
} from '../../../utils/api-utils'
import Amount from '../../../utils/amount'

// hooks
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'

// Selectors
import {
  UISelectors,
  WalletSelectors
} from '../../../common/selectors'
import {
  useSafeUISelector,
  useSafeWalletSelector,
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'

// Queries
import {
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../common/slices/constants'

// types
import {
  BraveWallet,
  WalletAccountType,
  AccountButtonOptionsObjectType,
  AccountModalTypes
} from '../../../constants/types'

// options
import { AccountButtonOptions } from '../../../options/account-list-button-options'

// components
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'
import {
  AccountActionsMenu
} from '../wallet-menus/account-actions-menu'
import {
  CreateAccountIcon
} from '../../shared/create-account-icon/create-account-icon'
import {
  TokenIconsStack
} from '../../shared/icon-stacks/token-icons-stack'
import LoadingSkeleton from '../../shared/loading-skeleton'

// style
import {
  StyledWrapper,
  NameAndIcon,
  AccountMenuWrapper,
  AccountMenuButton,
  AccountMenuIcon,
  AccountBalanceText,
  AccountDescription
} from './style'

import {
  AccountAddressButton,
  AccountAndAddress,
  AccountNameButton,
  AddressAndButtonRow,
  CopyIcon
} from '../portfolio-account-item/style'

import {
  HorizontalSpace,
  Row
} from '../../shared/style'

export interface Props {
  onDelete?: () => void
  onClick: (account: WalletAccountType) => void
  account: WalletAccountType
}

export const AccountListItem = ({
  account,
  onClick
}: Props) => {
  // redux
  const dispatch = useDispatch()

  // selectors
  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )
  const defaultFiatCurrency = useSafeWalletSelector(
    WalletSelectors.defaultFiatCurrency
  )
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // state
  const [showAccountMenu, setShowAccountMenu] = React.useState<boolean>(false)

  // refs
  const accountMenuRef = React.useRef<HTMLDivElement>(null)

  // hooks
  useOnClickOutside(
    accountMenuRef,
    () => setShowAccountMenu(false),
    showAccountMenu
  )

  // methods
  const onSelectAccount = React.useCallback(() => {
    onClick(account)
  }, [onClick, account])

  const onRemoveAccount = React.useCallback(() => {
    dispatch(
      AccountsTabActions.setAccountToRemove({
        accountId: account.accountId,
        name: account.name
      })
    )
  }, [account])

  const onShowAccountsModal = React.useCallback((modalType: AccountModalTypes) => {
    dispatch(AccountsTabActions.setShowAccountModal(true))
    dispatch(AccountsTabActions.setAccountModalType(modalType))
    dispatch(AccountsTabActions.setSelectedAccount(account))
  }, [account, dispatch])

  const onClickButtonOption = React.useCallback((id: AccountModalTypes) => {
    if (id === 'details') {
      onSelectAccount()
      return
    }
    if (id === 'remove') {
      onRemoveAccount()
      return
    }
    onShowAccountsModal(id)
  }, [onSelectAccount, onRemoveAccount, onShowAccountsModal])

  // memos
  const accountsFungibleTokens = React.useMemo(() => {
    return userVisibleTokensInfo.filter((asset) => asset.visible)
      .filter((token) => token.coin === account.accountId.coin)
      .filter((token) =>
        !token.isErc721 && !token.isErc1155 && !token.isNft)
  }, [userVisibleTokensInfo, account])

  const tokensWithBalances = React.useMemo(() => {
    return accountsFungibleTokens
      .filter((token) => new Amount(getBalance(account, token)).gt(0))
  }, [accountsFungibleTokens])

  const tokenPriceIds = React.useMemo(() =>
    accountsFungibleTokens
      .map(token => getPriceIdForToken(token)),
    [accountsFungibleTokens]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length ? { ids: tokenPriceIds } : skipToken,
    querySubscriptionOptions60s
  )

  const accountsFiatValue = React.useMemo(() => {
    // Return an empty string to display a loading
    // skeleton while assets are populated.
    if (userVisibleTokensInfo.length === 0) {
      return Amount.empty()
    }
    // Return a 0 balance if the account has no
    // assets to display.
    if (
      accountsFungibleTokens
        .length === 0
    ) {
      return new Amount(0)
    }

    const amounts =
      accountsFungibleTokens
        .map((asset) => {
          const balance =
            getBalance(account, asset)
          return computeFiatAmount({
            spotPriceRegistry,
            value: balance,
            token: asset
          })
        })

    const reducedAmounts =
      amounts.reduce(function (a, b) {
        return a.plus(b)
      })

    return !reducedAmounts.isUndefined()
      ? reducedAmounts
      : Amount.empty()
  }, [
    account,
    userVisibleTokensInfo,
    accountsFungibleTokens
  ])

  const buttonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    // We are not able to remove a Derived account so we filter out this option.
    if (account.accountId.kind === BraveWallet.AccountKind.kDerived) {
      return AccountButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'remove')
    }
    // We are not able to fetch Private Keys for a Hardware account so we filter out this option.
    if (account.accountId.kind === BraveWallet.AccountKind.kHardware) {
      return AccountButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'privateKey')
    }
    return AccountButtonOptions
  }, [account])

  // render
  return (
    <StyledWrapper>
      <NameAndIcon>
        <CreateAccountIcon
          size='big'
          address={account.address}
          accountKind={account.accountId.kind}
          marginRight={16}
        />
        <AccountAndAddress>
          <AccountNameButton
            onClick={onSelectAccount}
          >
            {account.name}
          </AccountNameButton>
          <AddressAndButtonRow>
            <AccountAddressButton
              onClick={onSelectAccount}
            >
              {reduceAddress(account.address)}
            </AccountAddressButton>
            <CopyTooltip text={account.address}>
              <CopyIcon />
            </CopyTooltip>
          </AddressAndButtonRow>
          <AccountDescription>
            {getAccountTypeDescription(account.accountId.coin)}
          </AccountDescription>
        </AccountAndAddress>
      </NameAndIcon>
      <Row
        width='unset'
      >
        {accountsFiatValue.isUndefined() ? (
          <>
            {!isPanel &&
              <>
                <LoadingSkeleton width={60} height={14} />
                <HorizontalSpace space='26px' /></>
            }
            <LoadingSkeleton width={60} height={14} />
            <HorizontalSpace space='12px' />
          </>
        ) : (
          <>
            {!isPanel &&
              <TokenIconsStack tokens={tokensWithBalances} />
            }
            <AccountBalanceText
              textSize='14px'
              isBold={true}
            >
              {accountsFiatValue.formatAsFiat(defaultFiatCurrency)}
            </AccountBalanceText>
          </>
        )}
        <AccountMenuWrapper
          ref={accountMenuRef}
        >
          <AccountMenuButton
            onClick={() => setShowAccountMenu(prev => !prev)}
          >
            <AccountMenuIcon />
          </AccountMenuButton>
          {showAccountMenu &&
            <AccountActionsMenu
              onClick={onClickButtonOption}
              options={buttonOptions}
            />
          }
        </AccountMenuWrapper>
      </Row>
    </StyledWrapper>
  )
}

export default AccountListItem
