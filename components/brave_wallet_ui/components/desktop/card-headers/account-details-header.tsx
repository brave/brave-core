// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import {
  AccountButtonOptionsObjectType,
  AccountModalTypes,
  BraveWallet,
  WalletRoutes
} from '../../../constants/types'

// Options
import {
  AccountDetailsMenuOptions //
} from '../../../options/account-details-menu-options'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getBalance } from '../../../utils/balance-utils'
import {
  computeFiatAmount,
  getPriceIdForToken
} from '../../../utils/pricing-utils'
import { getAccountTypeDescription } from '../../../utils/account-utils'
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

// Queries
import {
  useGetTokenSpotPricesQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetUserTokensRegistryQuery
} from '../../../common/slices/api.slice'
import {
  selectAllVisibleUserAssetsFromQueryResult //
} from '../../../common/slices/entities/blockchain-token.entity'
import {
  TokenBalancesRegistry //
} from '../../../common/slices/entities/token-balance.entity'
import { querySubscriptionOptions60s } from '../../../common/slices/constants'

// Hooks
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'

// Components
import {
  CreateAccountIcon //
} from '../../shared/create-account-icon/create-account-icon'
import CopyTooltip from '../../shared/copy-tooltip/copy-tooltip'
import { AccountDetailsMenu } from '../wallet-menus/account-details-menu'
import { LoadingSkeleton } from '../../shared/loading-skeleton/index'

// Styled Components
import {
  AccountNameText,
  AddressText,
  AccountsNetworkText,
  AccountBalanceText,
  CopyIcon
} from './account-details-header.style'
import {
  MenuButton,
  MenuButtonIcon,
  MenuWrapper,
  HorizontalDivider
} from './shared-card-headers.style'
import { Row, Column, HorizontalSpace } from '../../shared/style'
import { Button, ButtonIcon } from './shared-panel-headers.style'

interface Props {
  account: BraveWallet.AccountInfo
  onClickMenuOption: (option: AccountModalTypes) => void
  tokenBalancesRegistry: TokenBalancesRegistry | undefined | null
}

export const AccountDetailsHeader = (props: Props) => {
  const { account, onClickMenuOption, tokenBalancesRegistry } = props

  // routing
  const history = useHistory()

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // Queries
  const { userVisibleTokensInfo } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: (result) => ({
      userVisibleTokensInfo: selectAllVisibleUserAssetsFromQueryResult(result)
    })
  })
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  // state
  const [showAccountDetailsMenu, setShowAccountDetailsMenu] =
    React.useState<boolean>(false)

  // refs
  const accountDetailsMenuRef = React.useRef<HTMLDivElement>(null)

  // hooks
  useOnClickOutside(
    accountDetailsMenuRef,
    () => setShowAccountDetailsMenu(false),
    showAccountDetailsMenu
  )

  // Memos
  const accountsFungibleTokens = React.useMemo(() => {
    return userVisibleTokensInfo
      .filter((asset) => asset.visible)
      .filter((token) => token.coin === account.accountId.coin)
      .filter((token) => !token.isErc721 && !token.isErc1155 && !token.isNft)
  }, [userVisibleTokensInfo, account])

  const tokenPriceIds = React.useMemo(
    () => accountsFungibleTokens.map((token) => getPriceIdForToken(token)),
    [accountsFungibleTokens]
  )

  const { data: spotPriceRegistry, isLoading: isLoadingSpotPrices } =
    useGetTokenSpotPricesQuery(
      tokenPriceIds.length && defaultFiatCurrency
        ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
        : skipToken,
      querySubscriptionOptions60s
    )

  const accountsFiatValue = React.useMemo(() => {
    // Return an empty string to display a loading
    // skeleton while assets are populated.
    if (userVisibleTokensInfo.length === 0 || isLoadingSpotPrices) {
      return Amount.empty()
    }
    // Return a 0 balance if the account has no
    // assets to display.
    if (accountsFungibleTokens.length === 0) {
      return new Amount(0)
    }

    const amounts = accountsFungibleTokens.map((asset) => {
      const balance = getBalance(
        account.accountId,
        asset,
        tokenBalancesRegistry
      )
      return computeFiatAmount({
        spotPriceRegistry,
        value: balance,
        token: asset
      })
    })

    const reducedAmounts = amounts.reduce(function (a, b) {
      return a.plus(b)
    })

    return !reducedAmounts.isUndefined() ? reducedAmounts : Amount.empty()
  }, [
    account,
    userVisibleTokensInfo,
    accountsFungibleTokens,
    tokenBalancesRegistry,
    spotPriceRegistry,
    isLoadingSpotPrices
  ])

  const menuOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    let options = AccountDetailsMenuOptions
    // We are not able to remove a Derived account
    // so we filter out this option.
    if (account.accountId.kind === BraveWallet.AccountKind.kDerived) {
      options = options.filter(
        (option: AccountButtonOptionsObjectType) => option.id !== 'remove'
      )
    }
    // We are not able to fetch Private Keys for
    // a Hardware account so we filter out this option.
    // BTC and ZEC are not yet supported.
    if (
      account.accountId.coin === BraveWallet.CoinType.BTC ||
      account.accountId.coin === BraveWallet.CoinType.ZEC ||
      account.accountId.kind === BraveWallet.AccountKind.kHardware
    ) {
      options = options.filter(
        (option: AccountButtonOptionsObjectType) => option.id !== 'privateKey'
      )
    }
    // We are currently not able to support viewing a
    // BTC or ZEC account on a block explorer.
    // Link to issue https://github.com/brave/brave-browser/issues/39699
    if (
      account.accountId.coin === BraveWallet.CoinType.BTC ||
      account.accountId.coin === BraveWallet.CoinType.ZEC
    ) {
      options = options.filter(
        (option: AccountButtonOptionsObjectType) => option.id !== 'explorer'
      )
    }
    return options
  }, [account])

  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.Accounts)
  }, [history])

  return (
    <Row
      padding={isPanel ? '16px' : '24px 0px'}
      justifyContent='space-between'
    >
      <Row width='unset'>
        {isPanel ? (
          <Row
            width='unset'
            margin='0px 12px 0px 0px'
          >
            <Button onClick={goBack}>
              <ButtonIcon name='carat-left' />
            </Button>
          </Row>
        ) : (
          <MenuButton
            marginRight={16}
            onClick={goBack}
          >
            <MenuButtonIcon
              size={16}
              name='arrow-left'
            />
          </MenuButton>
        )}
        <CreateAccountIcon
          account={account}
          size='huge'
          marginRight={8}
        />
        <Column alignItems='flex-start'>
          <AccountNameText
            textSize='16px'
            isBold={true}
          >
            {account.name}
          </AccountNameText>
          {account.address && (
            <Row
              width='unset'
              alignItems='center'
              justifyContent='flex-start'
            >
              <AddressText>{reduceAddress(account.address)}</AddressText>
              <CopyTooltip text={account.address}>
                <CopyIcon />
              </CopyTooltip>
            </Row>
          )}
          <AccountsNetworkText>
            {getAccountTypeDescription(account.accountId)}
          </AccountsNetworkText>
        </Column>
      </Row>

      <Row width='unset'>
        {!isPanel && (
          <>
            <Column alignItems='flex-end'>
              <AccountsNetworkText>
                {getLocale('braveWalletAccountBalance')}
              </AccountsNetworkText>
              {accountsFiatValue.isUndefined() ? (
                <LoadingSkeleton
                  width={120}
                  height={32}
                />
              ) : (
                <AccountBalanceText>
                  {accountsFiatValue.formatAsFiat(defaultFiatCurrency)}
                </AccountBalanceText>
              )}
            </Column>
            <HorizontalSpace space='16px' />
            <HorizontalDivider />
            <HorizontalSpace space='16px' />
          </>
        )}
        <MenuWrapper ref={accountDetailsMenuRef}>
          {isPanel ? (
            <Button onClick={() => setShowAccountDetailsMenu((prev) => !prev)}>
              <ButtonIcon name='more-vertical' />
            </Button>
          ) : (
            <MenuButton
              onClick={() => setShowAccountDetailsMenu((prev) => !prev)}
            >
              <MenuButtonIcon name='more-vertical' />
            </MenuButton>
          )}
          {showAccountDetailsMenu && (
            <AccountDetailsMenu
              options={menuOptions}
              onClickMenuOption={onClickMenuOption}
            />
          )}
        </MenuWrapper>
      </Row>
    </Row>
  )
}

export default AccountDetailsHeader
