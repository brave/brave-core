// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { AccountPageTabs, BraveWallet } from '$wallet/constants/types'

// Hooks
import { useExplorer } from '$wallet/common/hooks/explorer'
import { useOnClickOutside } from '$wallet/common/hooks/useOnClickOutside'

// Utils
import { reduceAddress } from '$wallet/utils/reduce-address'
import Amount from '$wallet/utils/amount'
import {
  computeFiatAmount,
  getPriceRequestsForTokens,
} from '$wallet/utils/pricing-utils'
import { makeAccountRoute } from '$wallet/utils/routes-utils'
import { getIsRewardsAccount } from '$wallet/utils/rewards_utils'
import {
  externalWalletProviderFromString, //
} from '../../../../../../brave_rewards/resources/shared/lib/external_wallet'
import { getLocale } from '$web-common/locale'

// Components
import {
  WithHideBalancePlaceholder, //
} from '$wallet/components/desktop/with-hide-balance-placeholder'
import {
  PortfolioAccountMenu, //
} from '$wallet/components/desktop/wallet-menus/portfolio-account-menu'
import {
  RewardsMenu, //
} from '$wallet/components/desktop/wallet-menus/rewards_menu'
import {
  PopupModal, //
} from '$wallet/components/desktop/popup-modals/index'
import {
  DepositModal, //
} from '$wallet/components/desktop/popup-modals/account-settings-modal/account-settings-modal'
import {
  CreateAccountIcon, //
} from '$wallet/components/shared/create-account-icon/create-account-icon'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetPolkadotAddressForNetworkQuery,
} from '$wallet/common/slices/api.slice'
import {
  usePersistedTokenSpotPricesQuery, //
} from '$wallet/common/hooks/use-persisted-spot-prices'
import { querySubscriptionOptions60s } from '$wallet/common/slices/constants'

// Styled Components
import { StyledWrapper, AccountButton } from './portfolio_account_item.style'
import {
  BraveRewardsIndicator,
  VerticalSpacer,
  Text,
  Row,
  Column,
  VerticalDivider,
} from '$wallet/components/shared/style'

interface Props {
  account: BraveWallet.AccountInfo
  asset: BraveWallet.BlockchainToken
  assetBalance: string
  assetNetwork?: BraveWallet.NetworkInfo | null
  hideBalances?: boolean
  isSellSupported: boolean
  showSellModal: () => void
}

export const PortfolioAccountItem = (props: Props) => {
  const {
    asset,
    assetBalance,
    account,
    assetNetwork,
    hideBalances,
    isSellSupported,
    showSellModal,
  } = props

  // Routing
  const history = useHistory()

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(assetNetwork)

  // State
  const [showDepositModal, setShowDepositModal] = React.useState<boolean>(false)

  // Refs
  const depositModalRef = React.useRef<HTMLDivElement>(null)

  // Memos & Computed
  const isRewardsAccount = getIsRewardsAccount(account.accountId)

  const externalProvider = isRewardsAccount
    ? externalWalletProviderFromString(account.accountId.uniqueKey)
    : null

  const formattedAssetBalance: string = React.useMemo(() => {
    return new Amount(assetBalance)
      .divideByDecimals(asset.decimals)
      .compactAsAsset(6, asset.symbol)
  }, [assetBalance, asset.decimals, asset.symbol])

  const tokenPriceRequests = React.useMemo(
    () => getPriceRequestsForTokens([asset]),
    [asset],
  )

  // Queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

  const { data: spotPrices = [] } = usePersistedTokenSpotPricesQuery(
    defaultFiatCurrency && tokenPriceRequests.length
      ? { requests: tokenPriceRequests, vsCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s,
  )

  const { data: polkadotAddress } = useGetPolkadotAddressForNetworkQuery(
    account.accountId.coin === BraveWallet.CoinType.DOT
      ? { accountId: account.accountId, chainId: asset.chainId }
      : skipToken,
  )

  const displayAddress = polkadotAddress ?? account.address

  const fiatBalance: Amount = React.useMemo(() => {
    return computeFiatAmount({
      spotPrices,
      value: assetBalance,
      token: asset,
    })
  }, [spotPrices, assetBalance, asset])

  const isAssetsBalanceZero = React.useMemo(() => {
    return new Amount(assetBalance).isZero()
  }, [assetBalance])

  const blockExplorerSupported = !!account.address

  // Methods
  const onSelectAccount = React.useCallback(() => {
    history.push(makeAccountRoute(account, AccountPageTabs.AccountAssetsSub))
  }, [history, account])

  const onViewAccountOnBlockExplorer = React.useCallback(
    () => onClickViewOnBlockExplorer('address', account.address)(),
    [account.address, onClickViewOnBlockExplorer],
  )

  // Hooks
  useOnClickOutside(
    depositModalRef,
    () => setShowDepositModal(false),
    showDepositModal,
  )

  return (
    <>
      <StyledWrapper isRewardsAccount={isRewardsAccount}>
        <AccountButton
          onClick={onSelectAccount}
          disabled={isRewardsAccount}
        >
          <Row width='unset'>
            <CreateAccountIcon
              size='huge'
              marginRight={12}
              account={account}
              externalProvider={externalProvider}
            />
            <Column alignItems='flex-start'>
              <Text
                textSize='14px'
                isBold={true}
                textColor='primary'
                textAlign='left'
              >
                {account.name}
              </Text>
              {isRewardsAccount && (
                <>
                  <VerticalSpacer space='6px' />
                  <BraveRewardsIndicator>
                    {getLocale(S.BRAVE_WALLET_BRAVE_REWARDS_TITLE)}
                  </BraveRewardsIndicator>
                </>
              )}
              {displayAddress && !isRewardsAccount && (
                <Text
                  textSize='12px'
                  isBold={false}
                  textColor='primary'
                  textAlign='left'
                >
                  {reduceAddress(displayAddress)}
                </Text>
              )}
            </Column>
          </Row>
          <Column
            alignItems='flex-end'
            margin='0px 12px 0px 0px'
          >
            <WithHideBalancePlaceholder
              size='small'
              hideBalances={hideBalances ?? false}
            >
              <Text
                textSize='14px'
                isBold={true}
                textColor='primary'
                textAlign='right'
              >
                {formattedAssetBalance}
              </Text>
              <Text
                textSize='12px'
                isBold={false}
                textColor='secondary'
                textAlign='right'
              >
                {fiatBalance.compactAsFiat(defaultFiatCurrency)}
              </Text>
            </WithHideBalancePlaceholder>
          </Column>
        </AccountButton>
        {isRewardsAccount ? (
          <RewardsMenu />
        ) : (
          <PortfolioAccountMenu
            onClickViewOnExplorer={
              blockExplorerSupported ? onViewAccountOnBlockExplorer : undefined
            }
            onClickSell={
              isSellSupported && !isAssetsBalanceZero
                ? showSellModal
                : undefined
            }
            onClickDeposit={() => setShowDepositModal(true)}
          />
        )}
      </StyledWrapper>

      {showDepositModal && (
        <PopupModal
          title={getLocale(S.BRAVE_WALLET_DEPOSIT_CRYPTO_BUTTON)}
          onClose={() => setShowDepositModal(false)}
          ref={depositModalRef}
        >
          <VerticalDivider />
          <Column
            fullHeight={true}
            fullWidth={true}
            justifyContent='flex-start'
            padding='20px 15px'
          >
            <DepositModal selectedAccount={account} />
          </Column>
        </PopupModal>
      )}
    </>
  )
}
