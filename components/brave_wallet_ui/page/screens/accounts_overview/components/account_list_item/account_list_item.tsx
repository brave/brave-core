// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// redux
import { useAppDispatch } from '$wallet/common/hooks/use-redux'

// actions
import { AccountsTabActions } from '$wallet/page/reducers/accounts-tab-reducer'

// constants
import { emptyRewardsInfo } from '$wallet/common/async/base-query-cache'

// utils
import { reduceAddress } from '$wallet/utils/reduce-address'
import { getAccountTypeDescription } from '$wallet/utils/account-utils'
import { getBalance } from '$wallet/utils/balance-utils'
import { computeFiatAmount } from '$wallet/utils/pricing-utils'
import Amount from '$wallet/utils/amount'
import {
  getIsRewardsAccount,
  getIsRewardsToken,
  getRewardsTokenDescription,
} from '$wallet/utils/rewards_utils'
import { getLocale } from '$web-common/locale'
import { getEntitiesListFromEntityState } from '$wallet/utils/entities.utils'

// Selectors
import { UISelectors, WalletSelectors } from '$wallet/common/selectors'
import {
  useSafeUISelector,
  useSafeWalletSelector,
} from '$wallet/common/hooks/use-safe-selector'

// Queries
import {
  TokenBalancesRegistry, //
} from '$wallet/common/slices/entities/token-balance.entity'
import {
  useAddHiddenAccountMutation,
  useCanHideAccountQuery,
  useGetChainTipStatusQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetRewardsInfoQuery,
  useGetUserTokensRegistryQuery,
  useGetZCashAccountInfoQuery,
} from '$wallet/common/slices/api.slice'

// types
import {
  BraveWallet,
  AccountButtonOptionsObjectType,
  AccountModalTypes,
  WalletStatus,
} from '$wallet/constants/types'

// options
import { AccountButtonOptions } from '$wallet/options/account-list-button-options'

// components
import {
  AccountActionsMenu, //
} from '$wallet/components/desktop/wallet-menus/account-actions-menu'
import { RewardsMenu } from '$wallet/components/desktop/wallet-menus/rewards_menu'
import {
  CreateAccountIcon, //
} from '$wallet/components/shared/create-account-icon/create-account-icon'
import { TokenIconsStack } from '$wallet/components/shared/icon-stacks/token-icons-stack'
import LoadingSkeleton from '$wallet/components/shared/loading-skeleton'
import { RewardsLogin } from '../rewards_login/rewards_login'
import {
  ShieldZCashAccountModal, //
} from '$wallet/components/desktop/popup-modals/shield_zcash_account/shield_zcash_account'
import { ShieldedLabel } from '$wallet/components/shared/shielded_label/shielded_label'

// style
import {
  StyledWrapper,
  NameAndIcon,
  AccountBalanceText,
  AccountNameWrapper,
  AccountButton,
  WarningIcon,
} from './account_list_item.style'

import {
  HorizontalSpace,
  Row,
  BraveRewardsIndicator,
  VerticalSpacer,
  Text,
  Column,
} from '$wallet/components/shared/style'

interface Props {
  onDelete?: () => void
  onClick: (account: BraveWallet.AccountInfo) => void
  account: BraveWallet.AccountInfo
  tokenBalancesRegistry: TokenBalancesRegistry | undefined | null
  isLoadingBalances: boolean
  spotPrices: BraveWallet.AssetPrice[] | undefined
  isLoadingSpotPrices: boolean
  isShieldingAvailable: boolean | undefined
}

export const AccountListItem = ({
  account,
  onClick,
  tokenBalancesRegistry,
  spotPrices,
  isLoadingBalances,
  isLoadingSpotPrices,
  isShieldingAvailable,
}: Props) => {
  // redux
  const dispatch = useAppDispatch()

  // selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isMobile = useSafeUISelector(UISelectors.isMobile)

  // redux
  const isZCashShieldedTransactionsEnabled = useSafeWalletSelector(
    WalletSelectors.isZCashShieldedTransactionsEnabled,
  )

  // queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()
  const { data: userTokensRegistry } = useGetUserTokensRegistryQuery()
  const { data: zcashAccountInfo } = useGetZCashAccountInfoQuery(
    isZCashShieldedTransactionsEnabled
      && account.accountId.coin === BraveWallet.CoinType.ZEC
      ? account.accountId
      : skipToken,
  )

  const {
    data: {
      balance: rewardsBalance,
      provider,
      status: rewardsStatus,
      rewardsToken,
    } = emptyRewardsInfo,
  } = useGetRewardsInfoQuery()

  // state
  const [showShieldAccountModal, setShowShieldAccountModal] =
    React.useState<boolean>(false)
  const [addHiddenAccount] = useAddHiddenAccountMutation()
  const { data: canHideAccount = false } = useCanHideAccountQuery({
    accountId: account.accountId,
  })

  // methods
  const onSelectAccount = React.useCallback(() => {
    onClick(account)
  }, [onClick, account])

  const onRemoveAccount = React.useCallback(() => {
    dispatch(
      AccountsTabActions.setAccountToRemove({
        accountId: account.accountId,
        name: account.name,
      }),
    )
  }, [account, dispatch])

  const onShowAccountsModal = React.useCallback(
    (modalType: AccountModalTypes) => {
      dispatch(AccountsTabActions.setShowAccountModal(true))
      dispatch(AccountsTabActions.setAccountModalType(modalType))
      dispatch(AccountsTabActions.setSelectedAccount(account))
    },
    [account, dispatch],
  )

  const onClickButtonOption = React.useCallback(
    (id: AccountModalTypes) => {
      if (id === 'details') {
        onSelectAccount()
        return
      }
      if (id === 'hide') {
        void addHiddenAccount({ accountId: account.accountId })
        return
      }
      if (id === 'remove') {
        onRemoveAccount()
        return
      }
      if (id === 'shield' || id === 'resetBirthday') {
        setShowShieldAccountModal(true)
        return
      }
      onShowAccountsModal(id)
    },
    [
      account,
      addHiddenAccount,
      onSelectAccount,
      onRemoveAccount,
      onShowAccountsModal,
    ],
  )

  // memos & computed
  const isRewardsAccount = getIsRewardsAccount(account.accountId)

  const isDisconnectedRewardsAccount =
    isRewardsAccount && rewardsStatus === WalletStatus.kLoggedOut

  const externalProvider = isRewardsAccount ? provider : undefined

  const isShieldedAccount =
    isZCashShieldedTransactionsEnabled
    && !!zcashAccountInfo
    && !!zcashAccountInfo.accountShieldBirthday

  const { data: chainTipStatus } = useGetChainTipStatusQuery(
    isShieldedAccount ? account.accountId : skipToken,
  )

  const blocksBehind = chainTipStatus
    ? chainTipStatus.chainTip - chainTipStatus.latestScannedBlock
    : 0

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
      ],
    )
  }, [userTokensRegistry, account, isRewardsAccount, rewardsToken])

  const tokensWithBalances = React.useMemo(() => {
    if (isRewardsAccount && rewardsToken && rewardsBalance) {
      return [rewardsToken]
    }
    return accountsFungibleTokens.filter((token) =>
      new Amount(
        getBalance(account.accountId, token, tokenBalancesRegistry),
      ).gt(0),
    )
  }, [
    accountsFungibleTokens,
    tokenBalancesRegistry,
    account,
    isRewardsAccount,
    rewardsToken,
    rewardsBalance,
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
      accountsFungibleTokens.length === 0
      && !isLoadingBalances
      && !isLoadingSpotPrices
    ) {
      return new Amount(0)
    }

    // Wait for spot prices
    if (!spotPrices) {
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
        spotPrices,
        value: balance,
        token: asset,
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
    spotPrices,
    rewardsBalance,
    isLoadingBalances,
    isLoadingSpotPrices,
  ])

  const buttonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    // We are not able to remove a Derived account so we filter out this option.
    const canRemove =
      account.accountId.kind !== BraveWallet.AccountKind.kDerived

    // We are not able to fetch Private Keys for a Hardware account so we filter
    // out this option. Also PK export is allowed only for ETH, SOL, FIL and DOT.
    const canExportPrivateKey =
      [
        // TODO(apaymyshev): support BTC and ZEC
        BraveWallet.CoinType.ETH,
        BraveWallet.CoinType.SOL,
        BraveWallet.CoinType.FIL,
        BraveWallet.CoinType.DOT,
      ].includes(account.accountId.coin)
      && account.accountId.kind !== BraveWallet.AccountKind.kHardware

    const canShieldAccount =
      isZCashShieldedTransactionsEnabled
      && account.accountId.coin === BraveWallet.CoinType.ZEC
      && isShieldingAvailable
      && zcashAccountInfo
      && !zcashAccountInfo.accountShieldBirthday
    const canToggleHiddenAccount = canHideAccount

    const canResetShieldedAccountBirthday =
      isZCashShieldedTransactionsEnabled
      && account.accountId.coin === BraveWallet.CoinType.ZEC
      && zcashAccountInfo
      && !!zcashAccountInfo.accountShieldBirthday

    let options = [...AccountButtonOptions]
    options = options.map((option) =>
      option.id === 'hide'
        ? {
            ...option,
            id: 'hide',
            name: S.BRAVE_WALLET_ACCOUNTS_HIDE,
            icon: 'eye-off',
          }
        : option,
    )

    if (!canRemove) {
      options = options.filter((option) => option.id !== 'remove')
    }
    if (!canExportPrivateKey) {
      options = options.filter((option) => option.id !== 'privateKey')
    }
    if (!canShieldAccount) {
      options = options.filter((option) => option.id !== 'shield')
    }
    if (!canToggleHiddenAccount) {
      options = options.filter((option) => option.id !== 'hide')
    }
    if (!canResetShieldedAccountBirthday) {
      options = options.filter((option) => option.id !== 'resetBirthday')
    }
    return options
  }, [
    account,
    canHideAccount,
    isZCashShieldedTransactionsEnabled,
    isShieldingAvailable,
    zcashAccountInfo,
  ])

  const showSyncWarning =
    isShieldedAccount && (blocksBehind > 1000 || chainTipStatus === null)

  // render
  return (
    <>
      <StyledWrapper
        isRewardsAccount={isRewardsAccount}
        isOutOfSync={showSyncWarning}
      >
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
                    variant='default.semibold'
                    textColor='primary'
                    textAlign='left'
                  >
                    {account.name}
                  </Text>
                  <HorizontalSpace space='6px' />
                  {isShieldedAccount && <ShieldedLabel />}
                  {isRewardsAccount && (
                    <>
                      <VerticalSpacer space='4px' />
                      <BraveRewardsIndicator>
                        {getLocale(S.BRAVE_WALLET_BRAVE_REWARDS_TITLE)}
                      </BraveRewardsIndicator>
                      <VerticalSpacer space='4px' />
                    </>
                  )}
                </AccountNameWrapper>
                {account.address && !isRewardsAccount && (
                  <Text
                    variant='small.regular'
                    textColor='primary'
                    textAlign='left'
                  >
                    {reduceAddress(account.address)}
                  </Text>
                )}
                <Text
                  variant='small.regular'
                  textColor='secondary'
                  textAlign='left'
                >
                  {isRewardsAccount
                    ? getRewardsTokenDescription(externalProvider ?? null)
                    : getAccountTypeDescription(account.accountId)}
                </Text>
                {showSyncWarning && (
                  <Row
                    justifyContent='flex-start'
                    gap='4px'
                  >
                    <WarningIcon />
                    <Text
                      textColor='warning'
                      variant='default.regular'
                    >
                      {getLocale(S.BRAVE_WALLET_OUT_OF_SYNC_TITLE)}
                    </Text>
                  </Row>
                )}
              </Column>
            </NameAndIcon>

            {!isDisconnectedRewardsAccount && (
              <Row width='unset'>
                {!isMobile && !isPanel && !accountsFiatValue.isZero() ? (
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
                      textColor='primary'
                      variant='default.semibold'
                    >
                      {accountsFiatValue.compactAsFiat(defaultFiatCurrency)}
                    </AccountBalanceText>
                  </>
                )}
              </Row>
            )}
          </AccountButton>

          {!isDisconnectedRewardsAccount && (
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
