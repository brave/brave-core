// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Label from '@brave/leo/react/label'
import Icon from '@brave/leo/react/icon'

// redux
import { useDispatch } from 'react-redux'

// actions
import { AccountsTabActions } from '../../../page/reducers/accounts-tab-reducer'

// constants
import { emptyRewardsInfo } from '../../../common/async/base-query-cache'

// utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getAccountTypeDescription } from '../../../utils/account-utils'
import { getBalance } from '../../../utils/balance-utils'
import { computeFiatAmount } from '../../../utils/pricing-utils'
import Amount from '../../../utils/amount'
import {
  getIsRewardsAccount,
  getIsRewardsToken,
  getRewardsTokenDescription
} from '../../../utils/rewards_utils'
import { getLocale } from '../../../../common/locale'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'

// hooks
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'

// Selectors
import { UISelectors, WalletSelectors } from '../../../common/selectors'
import {
  useSafeUISelector,
  useSafeWalletSelector
} from '../../../common/hooks/use-safe-selector'

// Queries
import {
  TokenBalancesRegistry //
} from '../../../common/slices/entities/token-balance.entity'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetRewardsInfoQuery,
  useGetUserTokensRegistryQuery,
  useGetZCashAccountInfoQuery
} from '../../../common/slices/api.slice'

// types
import {
  BraveWallet,
  AccountButtonOptionsObjectType,
  AccountModalTypes,
  SpotPriceRegistry,
  WalletStatus
} from '../../../constants/types'

// options
import { AccountButtonOptions } from '../../../options/account-list-button-options'

// components
import {
  AccountActionsMenu //
} from '../wallet-menus/account-actions-menu'
import { RewardsMenu } from '../wallet-menus/rewards_menu'
import {
  CreateAccountIcon //
} from '../../shared/create-account-icon/create-account-icon'
import { TokenIconsStack } from '../../shared/icon-stacks/token-icons-stack'
import LoadingSkeleton from '../../shared/loading-skeleton'
import { RewardsLogin } from '../rewards_login/rewards_login'
import {
  ShieldZCashAccountModal //
} from '../popup-modals/shield_zcash_account/shield_zcash_account'

// style
import {
  StyledWrapper,
  NameAndIcon,
  AccountMenuWrapper,
  AccountMenuButton,
  AccountMenuIcon,
  AccountBalanceText,
  AccountDescription,
  AccountNameWrapper,
  AccountButton
} from './style'

import {
  HorizontalSpace,
  Row,
  BraveRewardsIndicator,
  VerticalSpacer,
  Text,
  Column
} from '../../shared/style'

interface Props {
  onDelete?: () => void
  onClick: (account: BraveWallet.AccountInfo) => void
  account: BraveWallet.AccountInfo
  tokenBalancesRegistry: TokenBalancesRegistry | undefined | null
  isLoadingBalances: boolean
  spotPriceRegistry: SpotPriceRegistry | undefined
  isLoadingSpotPrices: boolean
}

export const AccountListItem = ({
  account,
  onClick,
  tokenBalancesRegistry,
  spotPriceRegistry,
  isLoadingBalances,
  isLoadingSpotPrices
}: Props) => {
  // redux
  const dispatch = useDispatch()

  // selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  // redux
  const isZCashShieldedTransactionsEnabled = useSafeWalletSelector(
    WalletSelectors.isZCashShieldedTransactionsEnabled
  )

  // queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()
  const { data: userTokensRegistry } = useGetUserTokensRegistryQuery()
  const { data: zCashAccountInfo } = useGetZCashAccountInfoQuery(
    isZCashShieldedTransactionsEnabled &&
      account.accountId.coin === BraveWallet.CoinType.ZEC
      ? account.accountId
      : skipToken
  )

  const {
    data: {
      balance: rewardsBalance,
      provider,
      status: rewardsStatus,
      rewardsToken
    } = emptyRewardsInfo
  } = useGetRewardsInfoQuery()

  // state
  const [showAccountMenu, setShowAccountMenu] = React.useState<boolean>(false)
  const [showShieldAccountModal, setShowShieldAccountModal] =
    React.useState<boolean>(false)

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
  }, [account, dispatch])

  const onShowAccountsModal = React.useCallback(
    (modalType: AccountModalTypes) => {
      dispatch(AccountsTabActions.setShowAccountModal(true))
      dispatch(AccountsTabActions.setAccountModalType(modalType))
      dispatch(AccountsTabActions.setSelectedAccount(account))
    },
    [account, dispatch]
  )

  const onClickButtonOption = React.useCallback(
    (id: AccountModalTypes) => {
      if (id === 'details') {
        onSelectAccount()
        return
      }
      if (id === 'remove') {
        onRemoveAccount()
        return
      }
      if (id === 'shield') {
        setShowShieldAccountModal(true)
        return
      }
      onShowAccountsModal(id)
    },
    [onSelectAccount, onRemoveAccount, onShowAccountsModal]
  )

  // memos & computed
  const isRewardsAccount = getIsRewardsAccount(account.accountId)

  const isDisconnectedRewardsAccount =
    isRewardsAccount && rewardsStatus === WalletStatus.kLoggedOut

  const externalProvider = isRewardsAccount ? provider : undefined

  const accountsFungibleTokens = React.useMemo(() => {
    if (isRewardsAccount && rewardsToken) {
      return [rewardsToken]
    }

    if (!userTokensRegistry) {
      return []
    }

    return getEntitiesListFromEntityState(
      userTokensRegistry,
      userTokensRegistry.fungibleVisibleTokenIdsByCoinType[
        account.accountId.coin
      ]
    )
  }, [userTokensRegistry, account, isRewardsAccount, rewardsToken])

  const tokensWithBalances = React.useMemo(() => {
    if (isRewardsAccount && rewardsToken && rewardsBalance) {
      return [rewardsToken]
    }
    return accountsFungibleTokens.filter((token) =>
      new Amount(
        getBalance(account.accountId, token, tokenBalancesRegistry)
      ).gt(0)
    )
  }, [
    accountsFungibleTokens,
    tokenBalancesRegistry,
    account,
    isRewardsAccount,
    rewardsToken,
    rewardsBalance
  ])

  const accountsFiatValue = React.useMemo(() => {
    // Return an empty string to display a loading
    // skeleton while assets are populated.
    if (!userTokensRegistry) {
      return Amount.empty()
    }

    // Return a 0 balance if the account has no
    // assets to display.
    if (
      accountsFungibleTokens.length === 0 &&
      !isLoadingBalances &&
      !isLoadingSpotPrices
    ) {
      return new Amount(0)
    }

    // Wait for spot prices
    if (!spotPriceRegistry) {
      return Amount.empty()
    }

    const amounts = accountsFungibleTokens.map((asset) => {
      const isRewardsToken = getIsRewardsToken(asset)
      const balance =
        isRewardsToken && rewardsBalance
          ? new Amount(rewardsBalance)
              .multiplyByDecimals(asset.decimals)
              .format()
          : getBalance(account.accountId, asset, tokenBalancesRegistry)
      return computeFiatAmount({
        spotPriceRegistry,
        value: balance,
        token: asset
      })
    })

    const reducedAmounts = amounts.reduce(function (a, b) {
      return a.plus(b)
    }, Amount.empty())

    return !reducedAmounts.isUndefined() ? reducedAmounts : Amount.empty()
  }, [
    account,
    userTokensRegistry,
    accountsFungibleTokens,
    tokenBalancesRegistry,
    spotPriceRegistry,
    rewardsBalance,
    isLoadingBalances,
    isLoadingSpotPrices
  ])

  const buttonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    // We are not able to remove a Derived account so we filter out this option.
    const canRemove =
      account.accountId.kind !== BraveWallet.AccountKind.kDerived

    // We are not able to fetch Private Keys for a Hardware account so we filter
    // out this option. Also PK export is allowed only for ETH, SOL and FIL.
    const canExportPrivateKey =
      [
        // TODO(apaymyshev): support BTC and ZEC
        BraveWallet.CoinType.ETH,
        BraveWallet.CoinType.SOL,
        BraveWallet.CoinType.FIL
      ].includes(account.accountId.coin) &&
      account.accountId.kind !== BraveWallet.AccountKind.kHardware

    const canShieldAccount =
      isZCashShieldedTransactionsEnabled &&
      account.accountId.coin === BraveWallet.CoinType.ZEC &&
      zCashAccountInfo &&
      !zCashAccountInfo.accountShieldBirthday

    let options = [...AccountButtonOptions]

    if (!canRemove) {
      options = options.filter((option) => option.id !== 'remove')
    }
    if (!canExportPrivateKey) {
      options = options.filter((option) => option.id !== 'privateKey')
    }
    if (!canShieldAccount) {
      options = options.filter((option) => option.id !== 'shield')
    }
    return options
  }, [account, isZCashShieldedTransactionsEnabled, zCashAccountInfo])

  // render
  return (
    <>
      <StyledWrapper isRewardsAccount={isRewardsAccount}>
        <Row justifyContent='space-between'>
          <AccountButton
            onClick={onSelectAccount}
            disabled={isRewardsAccount}
          >
            <NameAndIcon>
              <CreateAccountIcon
                size='huge'
                account={account}
                marginRight={16}
                externalProvider={externalProvider}
              />
              <Column
                alignItems='flex-start'
                justifyContent='center'
              >
                <AccountNameWrapper width='unset'>
                  <Text
                    textSize='14px'
                    isBold={true}
                    textColor='primary'
                    textAlign='left'
                  >
                    {account.name}
                  </Text>
                  <HorizontalSpace space='8px' />
                  {zCashAccountInfo &&
                    zCashAccountInfo.accountShieldBirthday && (
                      <Label color='neutral'>
                        <div slot='icon-before'>
                          <Icon name='shield-done' />
                        </div>
                        {getLocale('braveWalletShielded')}
                      </Label>
                    )}
                  {isRewardsAccount && (
                    <>
                      <VerticalSpacer space='4px' />
                      <BraveRewardsIndicator>
                        {getLocale('braveWalletBraveRewardsTitle')}
                      </BraveRewardsIndicator>
                      <VerticalSpacer space='4px' />
                    </>
                  )}
                </AccountNameWrapper>
                {account.address && !isRewardsAccount && (
                  <Text
                    textSize='12px'
                    isBold={false}
                    textColor='primary'
                    textAlign='left'
                  >
                    {reduceAddress(account.address)}
                  </Text>
                )}
                <AccountDescription>
                  {isRewardsAccount
                    ? getRewardsTokenDescription(externalProvider ?? null)
                    : getAccountTypeDescription(account.accountId)}
                </AccountDescription>
              </Column>
            </NameAndIcon>

            {!isDisconnectedRewardsAccount && (
              <Row width='unset'>
                {!isPanel && !accountsFiatValue.isZero() ? (
                  tokensWithBalances.length ? (
                    <TokenIconsStack tokens={tokensWithBalances} />
                  ) : (
                    <>
                      <LoadingSkeleton
                        width={60}
                        height={14}
                      />
                      <HorizontalSpace space='26px' />
                    </>
                  )
                ) : null}

                {accountsFiatValue.isUndefined() ? (
                  <>
                    <LoadingSkeleton
                      width={60}
                      height={14}
                    />
                    <HorizontalSpace space='12px' />
                  </>
                ) : (
                  <>
                    <AccountBalanceText
                      textSize='14px'
                      isBold={true}
                    >
                      {accountsFiatValue.formatAsFiat(defaultFiatCurrency)}
                    </AccountBalanceText>
                  </>
                )}
              </Row>
            )}
          </AccountButton>

          {!isDisconnectedRewardsAccount && (
            <AccountMenuWrapper ref={accountMenuRef}>
              <AccountMenuButton
                onClick={() => setShowAccountMenu((prev) => !prev)}
              >
                <AccountMenuIcon />
              </AccountMenuButton>
              {showAccountMenu && (
                <>
                  {isRewardsAccount ? (
                    <RewardsMenu />
                  ) : (
                    <AccountActionsMenu
                      onClick={onClickButtonOption}
                      options={buttonOptions}
                    />
                  )}
                </>
              )}
            </AccountMenuWrapper>
          )}
        </Row>
        {isDisconnectedRewardsAccount && (
          <Row padding='0px 0px 8px 8px'>
            <RewardsLogin provider={provider} />
          </Row>
        )}
      </StyledWrapper>
      {showShieldAccountModal && (
        <ShieldZCashAccountModal
          account={account}
          onClose={() => {
            setShowShieldAccountModal(false)
          }}
        />
      )}
    </>
  )
}

export default AccountListItem
