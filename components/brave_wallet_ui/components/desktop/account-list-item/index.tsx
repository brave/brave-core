// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

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
import Amount from '../../../utils/amount'
import {
  getIsRewardsAccount,
  getIsRewardsToken,
  getRewardsBATToken,
  getRewardsTokenDescription
} from '../../../utils/rewards_utils'
import { getLocale } from '../../../../common/locale'

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
  TokenBalancesRegistry
} from '../../../common/slices/entities/token-balance.entity'
import {
  useGetExternalRewardsWalletQuery,
  useGetRewardsBalanceQuery
} from '../../../common/slices/api.slice'

// types
import {
  BraveWallet,
  AccountButtonOptionsObjectType,
  AccountModalTypes,
  SpotPriceRegistry
} from '../../../constants/types'
import {
  WalletStatus
} from '../../../common/async/brave_rewards_api_proxy'

// options
import { AccountButtonOptions } from '../../../options/account-list-button-options'

// components
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'
import {
  AccountActionsMenu
} from '../wallet-menus/account-actions-menu'
import {
  RewardsMenu
} from '../wallet-menus/rewards_menu'
import {
  CreateAccountIcon
} from '../../shared/create-account-icon/create-account-icon'
import {
  TokenIconsStack
} from '../../shared/icon-stacks/token-icons-stack'
import LoadingSkeleton from '../../shared/loading-skeleton'
import {
  RewardsLogin
} from '../rewards_login/rewards_login'

// style
import {
  StyledWrapper,
  NameAndIcon,
  AccountMenuWrapper,
  AccountMenuButton,
  AccountMenuIcon,
  AccountBalanceText,
  AccountDescription,
  AccountNameWrapper
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
  Row,
  BraveRewardsIndicator,
  VerticalSpacer,
} from '../../shared/style'

interface Props {
  onDelete?: () => void
  onClick: (account: BraveWallet.AccountInfo) => void
  account: BraveWallet.AccountInfo
  tokenBalancesRegistry: TokenBalancesRegistry | undefined
  spotPriceRegistry: SpotPriceRegistry | undefined
}

export const AccountListItem = ({
  account,
  onClick,
  tokenBalancesRegistry,
  spotPriceRegistry
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

  // queries
  const { data: rewardsBalance } = useGetRewardsBalanceQuery()
  const { data: externalRewardsInfo } = useGetExternalRewardsWalletQuery()

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

  // memos & computed
  const isRewardsAccount = getIsRewardsAccount(account.accountId)

  const isDisconnectedRewardsAccount =
    isRewardsAccount &&
    externalRewardsInfo?.status === WalletStatus.kLoggedOut

  const externalProvider =
    isRewardsAccount
      ? externalRewardsInfo?.provider
      : undefined

  const rewardsToken = getRewardsBATToken(externalProvider)

  const accountsFungibleTokens = React.useMemo(() => {
    if (isRewardsAccount && rewardsToken) {
      return [rewardsToken]
    }
    return userVisibleTokensInfo.filter((asset) => asset.visible)
      .filter((token) => token.coin === account.accountId.coin)
      .filter((token) =>
        !token.isErc721 && !token.isErc1155 && !token.isNft)
  }, [
    userVisibleTokensInfo,
    account,
    isRewardsAccount,
    rewardsToken
  ])

  const tokensWithBalances = React.useMemo(() => {
    if (
      isRewardsAccount
      && rewardsToken
      && rewardsBalance
    ) {
      return [rewardsToken]
    }
    return accountsFungibleTokens
      .filter((token) =>
        new Amount(getBalance(account.accountId, token, tokenBalancesRegistry)).gt(0))
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

    // Wait for spot prices
    if (!spotPriceRegistry) {
      return Amount.empty()
    }

    const amounts =
      accountsFungibleTokens
        .map((asset) => {
          const isRewardsToken = getIsRewardsToken(asset)
          const balance =
            isRewardsToken &&
              rewardsBalance
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

    const reducedAmounts =
      amounts.reduce(function (a, b) {
        return a.plus(b)
      })

    return !reducedAmounts.isUndefined()
      ? reducedAmounts
      : new Amount(0)
  }, [
    account,
    userVisibleTokensInfo,
    accountsFungibleTokens,
    tokenBalancesRegistry,
    spotPriceRegistry,
    rewardsBalance
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
      <Row
        justifyContent='space-between'
      >
        <NameAndIcon>
          <CreateAccountIcon
            size='big'
            account={account}
            marginRight={16}
            externalProvider={externalProvider}
          />
          <AccountAndAddress>
            <AccountNameWrapper
              width='unset'
            >
              <AccountNameButton
                onClick={onSelectAccount}
                disabled={isRewardsAccount}
              >
                {account.name}
              </AccountNameButton>
              {isRewardsAccount &&
                <>
                  <VerticalSpacer space='4px' />
                  <BraveRewardsIndicator>
                    {getLocale('braveWalletBraveRewardsTitle')}
                  </BraveRewardsIndicator>
                  <VerticalSpacer space='4px' />
                </>
              }
            </AccountNameWrapper>
            {account.address && !isRewardsAccount && (
              <AddressAndButtonRow>
                <AccountAddressButton onClick={onSelectAccount}>
                  {reduceAddress(account.address)}
                </AccountAddressButton>
                <CopyTooltip text={account.address}>
                  <CopyIcon />
                </CopyTooltip>
              </AddressAndButtonRow>
            )}
            <AccountDescription>
              {
                isRewardsAccount ?
                  getRewardsTokenDescription(externalProvider ?? null)
                  : getAccountTypeDescription(account.accountId.coin)
              }
            </AccountDescription>
          </AccountAndAddress>
        </NameAndIcon>
        {!isDisconnectedRewardsAccount &&
          <Row width='unset'>
            {!isPanel && !accountsFiatValue.isZero() ? (
              tokensWithBalances.length ? (
                <TokenIconsStack tokens={tokensWithBalances} />
              ) : (
                <>
                  <LoadingSkeleton width={60} height={14} />
                  <HorizontalSpace space='26px' />
                </>
              )
            ) : null}

            {accountsFiatValue.isUndefined() ? (
              <>
                <LoadingSkeleton width={60} height={14} />
                <HorizontalSpace space='12px' />
              </>
            ) : (
              <>
                <AccountBalanceText textSize='14px' isBold={true}>
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
              }
            </AccountMenuWrapper>
          </Row>
        }
      </Row>
      {isDisconnectedRewardsAccount &&
        <>
          <VerticalSpacer space='12px' />
          <RewardsLogin
            externalRewardsInfo={externalRewardsInfo}
          />
        </>
      }
    </StyledWrapper>
  )
}

export default AccountListItem
