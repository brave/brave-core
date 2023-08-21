// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Utils
import {
  getPriceIdForToken
} from '../../../utils/api-utils'
import Amount from '../../../utils/amount'
import {
  getBalance
} from '../../../utils/balance-utils'
import {
  computeFiatAmount
} from '../../../utils/pricing-utils'

// Proxies
import getWalletPanelApiProxy from '../../../panel/wallet_panel_api_proxy'
import { useApiProxy } from '../../../common/hooks/use-api-proxy'

// Selectors
import {
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import {
  WalletSelectors
} from '../../../common/selectors'

// Types
import {
  BraveWallet,
  WalletOrigin
} from '../../../constants/types'

// Queries
import {
  useGetSelectedChainQuery,
  useGetVisibleNetworksQuery,
  useGetAccountInfosRegistryQuery,
  useGetTokenSpotPricesQuery,
  useGetDefaultFiatCurrencyQuery
} from '../../../common/slices/api.slice'
import {
  useSelectedAccountQuery
} from '../../../common/slices/api.slice.extra'
import {
  selectAllAccountInfosFromQuery
} from '../../../common/slices/entities/account-info.entity'
import {
  querySubscriptionOptions60s
} from '../../../common/slices/constants'

// Hooks
import {
  useOnClickOutside
} from '../../../common/hooks/useOnClickOutside'
import {
  useBalancesFetcher
} from '../../../common/hooks/use-balances-fetcher'

// Components
import {
  DAppConnectionMain
} from './dapp-connection-main'
import {
  DAppConnectionNetworks
} from './dapp-connection-networks'
import {
  DAppConnectionAccounts
} from './dapp-connection-accounts'
import {
  CreateNetworkIcon
} from '../../shared/create-network-icon'

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

export type DAppConnectionOptionsType =
  'networks' | 'accounts' | 'main'

export const DAppConnectionSettings = () => {

  // Selectors
  const connectedAccounts = useUnsafeWalletSelector(
    WalletSelectors.connectedAccounts
  )
  const activeOrigin =
    useUnsafeWalletSelector(WalletSelectors.activeOrigin)
  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )

  // State
  const [showSettings, setShowSettings] = React.useState<boolean>(false)
  const [selectedOption, setSelectedOption] =
    React.useState<DAppConnectionOptionsType>('main')
  const [isPermissionDenied, setIsPermissionDenied] =
    React.useState<boolean>(false)
  const [isSolanaConnected, setIsSolanaConnected] =
    React.useState<boolean>(false)

  // Refs
  const settingsMenuRef =
    React.useRef<HTMLDivElement>(null)

  // Queries
  const { currentData: selectedNetwork } = useGetSelectedChainQuery(undefined)
  const selectedCoin = selectedNetwork?.coin
  const { data: selectedAccount } = useSelectedAccountQuery()
  const { data: networks } = useGetVisibleNetworksQuery()
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: accounts } =
    useGetAccountInfosRegistryQuery(undefined,
      {
        selectFromResult: (res) => ({
          data: selectAllAccountInfosFromQuery(res)
        })
      })

  const {
    data: tokenBalancesRegistry
  } = useBalancesFetcher({
    accounts,
    networks
  })

  const tokenPriceIds = React.useMemo(() =>
    userVisibleTokensInfo
      .filter(
        (token) => !token.isErc721 &&
          !token.isErc1155 && !token.isNft)
      .map(token => getPriceIdForToken(token)),
    [userVisibleTokensInfo]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiatCurrency
      ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  // Proxies
  const { braveWalletService } = useApiProxy()

  // Hooks
  useOnClickOutside(
    settingsMenuRef,
    () => setShowSettings(false),
    showSettings
  )

  // Memos
  const isConnected = React.useMemo((): boolean => {
    if (activeOrigin.originSpec === WalletOrigin) {
      return true
    }
    if (!selectedAccount || isPermissionDenied) {
      return false
    }
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      return isSolanaConnected
    }
    return connectedAccounts
      .some(
        (accountId) => accountId.uniqueKey ===
          selectedAccount.accountId.uniqueKey
      )
  }, [
    connectedAccounts,
    selectedAccount,
    activeOrigin,
    selectedCoin,
    isSolanaConnected,
    isPermissionDenied
  ])

  const fungibleTokensByChainId = React.useMemo(() => {
    return userVisibleTokensInfo.filter((asset) => asset.visible)
      .filter((token) => token.chainId === selectedNetwork?.chainId)
      .filter((token) =>
        !token.isErc721 && !token.isErc1155 && !token.isNft)
  }, [userVisibleTokensInfo, selectedNetwork?.chainId])

  // Methods
  const onSelectOption = React.useCallback(
    (option: DAppConnectionOptionsType) => {
      setSelectedOption(option)
    }, [])

  const getAccountsFiatValue = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      // Return an empty string to display a loading
      // skeleton while assets are populated.
      if (userVisibleTokensInfo.length === 0) {
        return Amount.empty()
      }
      // Return a 0 balance if the account has no
      // assets to display.
      if (
        fungibleTokensByChainId
          .length === 0
      ) {
        return new Amount(0)
      }

      const amounts =
        fungibleTokensByChainId
          .map((asset) => {
            const balance =
              getBalance(account.accountId, asset, tokenBalancesRegistry)
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
    userVisibleTokensInfo,
    fungibleTokensByChainId,
    tokenBalancesRegistry,
    spotPriceRegistry
  ])

  // Effects
  React.useEffect(() => {
    let subscribed = true

    if (
      selectedAccount?.address &&
      selectedCoin === BraveWallet.CoinType.SOL
    ) {
      (async () => {
        const { panelHandler } = getWalletPanelApiProxy()
        await panelHandler
          .isSolanaAccountConnected(selectedAccount?.address)
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

  React.useEffect(() => {
    let subscribed = true

    if (selectedCoin) {
      (async () => {
        await braveWalletService
          .isPermissionDenied(selectedCoin)
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
  }, [braveWalletService, selectedCoin, activeOrigin])

  return (
    <>
      <SettingsButton
        onClick={() => setShowSettings(prev => !prev)}
        data-test-id='dapp-settings-button'
      >
        <ConnectedIcon
          name={
            isConnected
              ? 'check-circle-filled'
              : 'social-dribbble'
          }
          dappConnected={isConnected}
          size='12px'
        />
        <FavIcon
          size='20px'
          src={
            `chrome://favicon/size/64@1x/${activeOrigin.originSpec}`
          }
        />
        <NetworkIconWrapper>
          <CreateNetworkIcon
            network={selectedNetwork}
            size='tiny'
          />
        </NetworkIconWrapper>
      </SettingsButton>
      {showSettings &&
        <>
          <OverlapForClick />
          <SettingsBubbleWrapper>
            <Pointer />
            <PointerShadow />
            <SettingsBubble
              ref={settingsMenuRef}
            >
              {selectedOption === 'main' &&
                <DAppConnectionMain
                  onSelectOption={onSelectOption}
                  isConnected={isConnected}
                  isPermissionDenied={isPermissionDenied}
                  getAccountsFiatValue={getAccountsFiatValue}
                />
              }
              {selectedOption === 'networks' &&
                <DAppConnectionNetworks
                  onSelectOption={onSelectOption}
                />
              }
              {selectedOption === 'accounts' &&
                <DAppConnectionAccounts
                  onSelectOption={onSelectOption}
                  getAccountsFiatValue={getAccountsFiatValue}
                />
              }
            </SettingsBubble>
          </SettingsBubbleWrapper>
          <BackgroundBlur />
        </>
      }
    </>
  )
}
