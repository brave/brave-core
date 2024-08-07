// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Utils
import Amount from '../../../utils/amount'
import { getBalance } from '../../../utils/balance-utils'
import {
  computeFiatAmount,
  getPriceIdForToken
} from '../../../utils/pricing-utils'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'
import {
  networkEntityAdapter //
} from '../../../common/slices/entities/network.entity'

// Proxies
import getWalletPanelApiProxy from '../../../panel/wallet_panel_api_proxy'

// Types
import {
  BraveWallet,
  DAppConnectionOptionsType
} from '../../../constants/types'

// Queries
import {
  useGetSelectedChainQuery,
  useGetVisibleNetworksQuery,
  useGetAccountInfosRegistryQuery,
  useGetTokenSpotPricesQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetActiveOriginConnectedAccountIdsQuery,
  useGetUserTokensRegistryQuery,
  useGetActiveOriginQuery
} from '../../../common/slices/api.slice'
import {
  useSelectedAccountQuery //
} from '../../../common/slices/api.slice.extra'
import {
  selectAllAccountInfosFromQuery //
} from '../../../common/slices/entities/account-info.entity'
import { querySubscriptionOptions60s } from '../../../common/slices/constants'

// Hooks
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'
import { useBalancesFetcher } from '../../../common/hooks/use-balances-fetcher'

// Components
import { DAppConnectionMain } from './dapp-connection-main'
import { DAppConnectionNetworks } from './dapp-connection-networks'
import { DAppConnectionAccounts } from './dapp-connection-accounts'
import { CreateNetworkIcon } from '../../shared/create-network-icon'

// Styled Components
import {
  SettingsButton,
  ConnectedIcon,
  SettingsBubbleWrapper,
  SettingsBubble,
  Pointer,
  PointerShadow,
  BackgroundBlur,
  FavIcon,
  NetworkIconWrapper,
  OverlapForClick
} from './dapp-connection-settings.style'

export const DAppConnectionSettings = () => {
  // Selectors
  const { data: connectedAccounts = [] } =
    useGetActiveOriginConnectedAccountIdsQuery()

  // State
  const [showSettings, setShowSettings] = React.useState<boolean>(false)
  const [selectedOption, setSelectedOption] =
    React.useState<DAppConnectionOptionsType>('main')
  const [isPermissionDenied, setIsPermissionDenied] =
    React.useState<boolean>(false)
  const [isSolanaConnected, setIsSolanaConnected] =
    React.useState<boolean>(false)

  // Refs
  const settingsMenuRef = React.useRef<HTMLDivElement>(null)

  // Queries
  const { data: activeOrigin = { eTldPlusOne: '', originSpec: '' } } =
    useGetActiveOriginQuery()
  const { currentData: selectedNetwork } = useGetSelectedChainQuery(undefined)
  const selectedCoin = selectedNetwork?.coin
  const { data: selectedAccount } = useSelectedAccountQuery()
  const { data: networks } = useGetVisibleNetworksQuery()
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: userTokensRegistry } = useGetUserTokensRegistryQuery()
  const { data: accounts } = useGetAccountInfosRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      data: selectAllAccountInfosFromQuery(res)
    })
  })

  const fungibleTokensByChainId = React.useMemo(() => {
    if (!userTokensRegistry || !selectedNetwork) {
      return []
    }
    return getEntitiesListFromEntityState(
      userTokensRegistry,
      userTokensRegistry.fungibleVisibleTokenIdsByChainId[
        networkEntityAdapter.selectId(selectedNetwork)
      ]
    )
  }, [userTokensRegistry, selectedNetwork])

  const { data: tokenBalancesRegistry } = useBalancesFetcher({
    accounts,
    networks
  })

  const tokenPriceIds = React.useMemo(
    () => fungibleTokensByChainId.map(getPriceIdForToken),
    [fungibleTokensByChainId]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiatCurrency
      ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  // Hooks
  useOnClickOutside(settingsMenuRef, () => setShowSettings(false), showSettings)

  // Computed
  const isChromeOrigin = activeOrigin?.originSpec.startsWith('chrome')

  // Memos
  const isConnected = React.useMemo((): boolean => {
    if (!selectedAccount || isPermissionDenied) {
      return false
    }
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      return isSolanaConnected
    }
    return connectedAccounts.some(
      (accountId) => accountId.uniqueKey === selectedAccount.accountId.uniqueKey
    )
  }, [
    connectedAccounts,
    selectedAccount,
    selectedCoin,
    isSolanaConnected,
    isPermissionDenied
  ])

  // Methods
  const onSelectOption = React.useCallback(
    (option: DAppConnectionOptionsType) => {
      setSelectedOption(option)
    },
    []
  )

  const getAccountsFiatValue = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      // Return an empty string to display a loading
      // skeleton while assets are populated.
      if (!userTokensRegistry) {
        return Amount.empty()
      }
      // Return a 0 balance if the account has no
      // assets to display.
      if (fungibleTokensByChainId.length === 0) {
        return new Amount(0)
      }

      const amounts = fungibleTokensByChainId.map((asset) => {
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
    },
    [
      userTokensRegistry,
      fungibleTokensByChainId,
      tokenBalancesRegistry,
      spotPriceRegistry
    ]
  )

  // Effects
  React.useEffect(() => {
    let subscribed = true

    if (selectedAccount?.address && selectedCoin === BraveWallet.CoinType.SOL) {
      ;(async () => {
        const { panelHandler } = getWalletPanelApiProxy()
        await panelHandler
          .isSolanaAccountConnected(selectedAccount?.address)
          .then((result) => {
            if (subscribed) {
              setIsSolanaConnected(result.connected)
            }
          })
          .catch((e) => console.log(e))
      })()
    }

    return () => {
      subscribed = false
    }
  }, [selectedAccount?.address, selectedCoin])

  React.useEffect(() => {
    let subscribed = true

    if (selectedCoin) {
      ;(async () => {
        await getWalletPanelApiProxy()
          .braveWalletService.isPermissionDenied(selectedCoin)
          .then((result) => {
            if (subscribed) {
              setIsPermissionDenied(result.denied)
            }
          })
          .catch((e) => console.log(e))
      })()
    }

    return () => {
      subscribed = false
    }
  }, [selectedCoin, activeOrigin])

  return (
    <>
      <SettingsButton
        onClick={() => setShowSettings((prev) => !prev)}
        data-test-id='dapp-settings-button'
        showConnectionStatus={!isChromeOrigin}
      >
        {!isChromeOrigin && (
          <ConnectedIcon
            name={isConnected ? 'check-circle-filled' : 'social-dribbble'}
            dappConnected={isConnected}
            size='12px'
          />
        )}
        <FavIcon
          size='20px'
          src={`chrome://favicon/size/64@1x/${activeOrigin.originSpec}`}
        />
        <NetworkIconWrapper showConnectionStatus={!isChromeOrigin}>
          <CreateNetworkIcon
            network={selectedNetwork}
            size='tiny'
          />
        </NetworkIconWrapper>
      </SettingsButton>
      {showSettings && (
        <>
          <OverlapForClick />
          <SettingsBubbleWrapper>
            <Pointer />
            <PointerShadow />
            <SettingsBubble ref={settingsMenuRef}>
              {selectedOption === 'main' && (
                <DAppConnectionMain
                  onSelectOption={onSelectOption}
                  isConnected={isConnected}
                  isPermissionDenied={isPermissionDenied}
                  getAccountsFiatValue={getAccountsFiatValue}
                  isChromeOrigin={isChromeOrigin}
                />
              )}
              {selectedOption === 'networks' && (
                <DAppConnectionNetworks onSelectOption={onSelectOption} />
              )}
              {selectedOption === 'accounts' && (
                <DAppConnectionAccounts
                  onSelectOption={onSelectOption}
                  getAccountsFiatValue={getAccountsFiatValue}
                />
              )}
            </SettingsBubble>
          </SettingsBubbleWrapper>
          <BackgroundBlur />
        </>
      )}
    </>
  )
}
