// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Proxies
import getWalletPanelApiProxy from '../../../panel/wallet_panel_api_proxy'

// Components
import { background } from 'ethereum-blockies'
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'

// Utils
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import Amount from '../../../utils/amount'
import { makeNetworkAsset } from '../../../options/asset-options'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { getTokenPriceAmountFromRegistry } from '../../../utils/pricing-utils'
import { WalletSelectors } from '../../../common/selectors'
import { getBalance } from '../../../utils/balance-utils'

// Hooks
import { useExplorer } from '../../../common/hooks/explorer'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetSelectedChainQuery,
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../common/slices/constants'
import { useSelectedAccountQuery } from '../../../common/slices/api.slice.extra'
import { useApiProxy } from '../../../common/hooks/use-api-proxy'
import {
  useScopedBalanceUpdater
} from '../../../common/hooks/use-scoped-balance-updater'
import {
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import { useAccountOrb } from '../../../common/hooks/use-orb'

// types
import {
  PanelTypes,
  CoinType,
  WalletOrigin
} from '../../../constants/types'

// Components
import {
  ConnectedHeader
} from '../connected-header/index'
import {
  SelectNetworkButton //
} from '../../shared/select-network-button/select-network-button'
import { LoadingSkeleton } from '../../shared/loading-skeleton/index'
import { PanelBottomNav } from '../panel-bottom-nav/panel-bottom-nav'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  FiatBalanceText,
  AccountCircle,
  AccountAddressText,
  AccountNameText,
  CenterColumn,
  OvalButton,
  OvalButtonText,
  BigCheckMark,
  StatusRow,
  BalanceColumn,
  SwitchIcon,
  MoreAssetsButton,
  ConnectedStatusBubble
} from './style'

import { VerticalSpacer } from '../../shared/style'

interface Props {
  navAction: (path: PanelTypes) => void
}

export const ConnectedPanel = (props: Props) => {
  const {
    navAction
  } = props

  const originInfo = useUnsafeWalletSelector(WalletSelectors.activeOrigin)
  const connectedAccounts = useUnsafeWalletSelector(
    WalletSelectors.connectedAccounts
  )

  // queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { currentData: selectedNetwork } = useGetSelectedChainQuery(undefined)
  const selectedCoin = selectedNetwork?.coin
  const { data: selectedAccount } = useSelectedAccountQuery()

  const networkAsset = React.useMemo(() =>
    makeNetworkAsset(selectedNetwork),
    [selectedNetwork]
  )

  const {
    data: tokenBalancesRegistry,
    isLoading: isLoadingBalances,
    isFetching: isFetchingBalances
  } = useScopedBalanceUpdater(
    selectedNetwork && selectedAccount && networkAsset
      ? {
          network: selectedNetwork,
          accounts: [selectedAccount],
          tokens: [networkAsset]
        }
      : skipToken
    )

  const networkTokenPriceIds = React.useMemo(() =>
    networkAsset
      ? [getPriceIdForToken(networkAsset)]
      : [],
    [networkAsset]
  )

  const {
    data: spotPriceRegistry,
    isLoading: isLoadingSpotPrices
  } = useGetTokenSpotPricesQuery(
    networkTokenPriceIds.length && defaultFiatCurrency
      ? { ids: networkTokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  // state
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [isSolanaConnected, setIsSolanaConnected] = React.useState<boolean>(false)
  const [isPermissionDenied, setIsPermissionDenied] = React.useState<boolean>(false)

  // computed
  // TODO(apaymyshev): handle bitcoin address
  const selectedAccountAddress = selectedAccount?.address || ''
  const selectedAccountName = selectedAccount?.name || ''

  // custom hooks
  const { braveWalletService } = useApiProxy()
  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  // methods
  const navigate = React.useCallback((path: PanelTypes) => () => {
    navAction(path)
  }, [navAction])

  const onExpand = React.useCallback(() => {
    navAction('expanded')
  }, [navAction])

  const onShowSitePermissions = React.useCallback(() => {
    if (isPermissionDenied) {
      const contentPath = selectedCoin === CoinType.SOL ? 'solana' : 'ethereum'
      chrome.tabs.create({
        url: `brave://settings/content/${contentPath}`
      }).catch((e) => { console.error(e) })
      return
    }
    navAction('sitePermissions')
  }, [navAction, isPermissionDenied, selectedCoin])

  const onShowMore = React.useCallback(() => {
    setShowMore(true)
  }, [])

  const onHideMore = React.useCallback(() => {
    if (showMore) {
      setShowMore(false)
    }
  }, [showMore])

  // effects
  React.useEffect(() => {
    let subscribed = true

    if (selectedCoin) {
      (async () => {
        await braveWalletService.isPermissionDenied(selectedCoin)
          .then(result => {
            if (subscribed) {
              setIsPermissionDenied(result.denied)
            }
          })
          .catch(e => console.log(e))
      })()
    }

    return () => {
      subscribed = false
    }
    // Keep dependency on originInfo. When braveWalletService.isPermissionDenied
    // is moved to api slice it's cached value should be reset on activeOriginChanged event.
  }, [braveWalletService, selectedCoin, originInfo])

  React.useEffect(() => {
    let subscribed = true

    if (selectedAccount?.address && selectedCoin === CoinType.SOL) {
      (async () => {
        const { panelHandler } = getWalletPanelApiProxy()
        await panelHandler.isSolanaAccountConnected(selectedAccount?.address)
          .then(result => {
            if (subscribed) {
              setIsSolanaConnected(result.connected)
            }
          })
          .catch(e => console.log(e))
      })()
    }

    return () => {
      subscribed = false
    }
  }, [selectedAccount?.address, selectedCoin])

  // memos
  const bg = React.useMemo(() => {
    return background({ seed: selectedAccountAddress.toLowerCase() })
  }, [selectedAccountAddress])

  const orb = useAccountOrb(selectedAccount)

  const selectedAccountBalance = React.useMemo(() => {
    if (
      !tokenBalancesRegistry ||
      !networkAsset ||
      !selectedAccount ||
      isLoadingBalances ||
      isFetchingBalances
    ) {
      return Amount.empty()
    }

    const balance = getBalance(
      selectedAccount.accountId,
      networkAsset,
      tokenBalancesRegistry
    )

    return new Amount(balance).divideByDecimals(networkAsset.decimals)
  }, [
    tokenBalancesRegistry,
    networkAsset,
    selectedAccount,
    isLoadingBalances,
    isFetchingBalances
  ])

  const selectedAccountFiatBalance = React.useMemo(() => {
    if (
      selectedAccountBalance.isUndefined() ||
      !networkAsset ||
      !spotPriceRegistry ||
      isLoadingSpotPrices
    ) {
      return Amount.empty()
    }

    return selectedAccountBalance
      .times(getTokenPriceAmountFromRegistry(spotPriceRegistry, networkAsset))
  }, [
    selectedAccountBalance,
    networkAsset,
    spotPriceRegistry,
    isLoadingSpotPrices
  ])

  const isConnected = React.useMemo((): boolean => {
    if (!selectedAccount) {
      return false
    }
    if (selectedCoin === CoinType.SOL) {
      return isSolanaConnected
    }
    if (originInfo.originSpec === WalletOrigin) {
      return true
    } else {
      return connectedAccounts.some(
        (accountId) => accountId.uniqueKey === selectedAccount.accountId.uniqueKey
      )
    }
  }, [connectedAccounts, selectedAccount, originInfo, selectedCoin, isSolanaConnected])

  const connectedStatusText = React.useMemo((): string => {
    if (isPermissionDenied) {
      return getLocale('braveWalletPanelBlocked')
    }
    if (selectedCoin === CoinType.SOL) {
      return isConnected
        ? getLocale('braveWalletPanelConnected')
        : getLocale('braveWalletPanelDisconnected')
    }
    return isConnected
      ? getLocale('braveWalletPanelConnected')
      : getLocale('braveWalletPanelNotConnected')
  }, [isConnected, selectedCoin, isPermissionDenied])

  const showConnectButton = React.useMemo((): boolean => {
    if (isPermissionDenied) {
      return true
    }
    if (selectedCoin === CoinType.SOL) {
      return connectedAccounts.length !== 0
    }
    if (!originInfo)
      return false

    return !originInfo.originSpec.startsWith('chrome')
  }, [selectedCoin, connectedAccounts, originInfo, isPermissionDenied])

  // computed
  const formattedAssetBalance = React.useMemo(() => {
    if (
      selectedAccountBalance.isUndefined() ||
      !networkAsset
    ) {
      return ''
    }

    return selectedAccountBalance.formatAsAsset(6, networkAsset.symbol)
  }, [
    selectedAccountBalance,
    networkAsset
  ])

  // render
  return (
    <StyledWrapper onClick={onHideMore} panelBackground={bg}>
      <ConnectedHeader
        onExpand={onExpand}
        onClickMore={onShowMore}
        onClickViewOnBlockExplorer={selectedAccount ? onClickViewOnBlockExplorer('address', selectedAccountAddress) : undefined}
        showMore={showMore}
      />

      <CenterColumn>

        <StatusRow>
          <SelectNetworkButton
            onClick={navigate('networks')}
            selectedNetwork={selectedNetwork}
            isPanel={true}
          />
        </StatusRow>

        {showConnectButton ? (
          <StatusRow>
            <OvalButton onClick={onShowSitePermissions}>
              {selectedCoin === CoinType.SOL ? (
                <ConnectedStatusBubble isConnected={isConnected} />
              ) : (
                <>
                  {isConnected && <BigCheckMark />}
                </>
              )}
              <OvalButtonText>{connectedStatusText}</OvalButtonText>
            </OvalButton>
          </StatusRow>
        ) : (
          <div />
        )}

        <VerticalSpacer space='8px' />

        <BalanceColumn>
          <AccountCircle orb={orb} onClick={navigate('accounts')}>
            <SwitchIcon />
          </AccountCircle>
          <AccountNameText>{reduceAccountDisplayName(selectedAccountName, 24)}</AccountNameText>
          <CopyTooltip text={selectedAccountAddress}>
            <AccountAddressText>{reduceAddress(selectedAccountAddress)}</AccountAddressText>
          </CopyTooltip>
        </BalanceColumn>
        <BalanceColumn>
          {formattedAssetBalance ? (
            <AssetBalanceText>{formattedAssetBalance}</AssetBalanceText>
          ) : (
            <>
              <VerticalSpacer space={6} />
              <LoadingSkeleton useLightTheme={true} width={120} height={24} />
              <VerticalSpacer space={6} />
            </>
          )}
          {!selectedAccountFiatBalance.isUndefined() ? (
            <FiatBalanceText>
              {selectedAccountFiatBalance.formatAsFiat(defaultFiatCurrency)}
            </FiatBalanceText>
          ) : (
            <LoadingSkeleton useLightTheme={true} width={80} height={20} />
          )}
        </BalanceColumn>
        <MoreAssetsButton onClick={navigate('assets')}>{getLocale('braveWalletPanelViewAccountAssets')}</MoreAssetsButton>
      </CenterColumn>
      <PanelBottomNav
        onNavigate={navAction}
      />
    </StyledWrapper>
  )
}

export default ConnectedPanel
