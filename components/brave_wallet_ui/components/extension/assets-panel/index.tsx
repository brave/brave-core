// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

import { BraveWallet, WalletRoutes } from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'
import { getAssetIdKey } from '../../../utils/asset-utils'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { getTokenPriceAmountFromRegistry } from '../../../utils/pricing-utils'
import Amount from '../../../utils/amount'
import { getBalance } from '../../../utils/balance-utils'

// Styled Components
import { StyledWrapper, AddAssetButton } from './style'

// RTK
import {
  useGetDefaultFiatCurrencyQuery,
  useGetSelectedChainQuery,
  useGetTokenSpotPricesQuery,
  useGetUserTokensRegistryQuery
} from '../../../common/slices/api.slice'
import { querySubscriptionOptions60s } from '../../../common/slices/constants'
import {
  makeSelectAllUserAssetsForChainFromQueryResult //
} from '../../../common/slices/entities/blockchain-token.entity'

// Hooks
import {
  useScopedBalanceUpdater //
} from '../../../common/hooks/use-scoped-balance-updater'

// Components
import { PortfolioAssetItem } from '../../desktop/portfolio-asset-item/index'

interface Props {
  selectedAccount?: BraveWallet.AccountInfo
  onAddAsset: () => void
}

export const AssetsPanel = (props: Props) => {
  const { selectedAccount, onAddAsset } = props

  const routeToAssetDetails = (url: string) => {
    chrome.tabs.create({ url: url }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { currentData: selectedNetwork } = useGetSelectedChainQuery(undefined)

  const userTokensByChainIdSelector = React.useMemo(() => {
    return makeSelectAllUserAssetsForChainFromQueryResult()
  }, [])

  const { data: userTokens } = useGetUserTokensRegistryQuery(
    selectedNetwork ? undefined : skipToken,
    {
      selectFromResult: (res) => ({
        ...res,
        data: selectedNetwork
          ? userTokensByChainIdSelector(res, selectedNetwork.chainId)
          : undefined
      })
    }
  )

  const {
    data: tokenBalancesRegistry,
    isLoading: isLoadingBalances,
    isFetching: isFetchingBalances
  } = useScopedBalanceUpdater(
    selectedNetwork && selectedAccount
      ? {
          network: selectedNetwork,
          accounts: [selectedAccount],
          tokens:
            selectedNetwork.coin === BraveWallet.CoinType.SOL
              ? // Use optimised balance scanner for SOL, which doesn't need a
                // reference tokens list.
                undefined
              : // ETH needs a reference tokens list to scan for balances.
                // If this is undefined, for example while userTokens is being
                // fetched, then the hook will skip the query.
                userTokens
        }
      : skipToken
  )

  const onClickAsset = React.useCallback(
    (
        contractAddress: string,
        symbol: string,
        tokenId: string,
        chainId: string
      ) =>
      () => {
        if (contractAddress === '') {
          routeToAssetDetails(
            `brave://wallet${
              WalletRoutes.PortfolioAssets //
            }/${
              chainId //
            }/${symbol}`
          )
          return
        }
        if (tokenId !== '') {
          routeToAssetDetails(
            `brave://wallet${
              WalletRoutes.PortfolioNFTs //
            }/${
              chainId //
            }/${
              contractAddress //
            }/${tokenId}`
          )
          return
        }
        routeToAssetDetails(
          `brave://wallet${
            WalletRoutes.PortfolioAssets //
          }/${
            chainId //
          }/${contractAddress}`
        )
      },
    [routeToAssetDetails]
  )

  const tokenPriceIds = React.useMemo(
    () =>
      selectedAccount && userTokens && tokenBalancesRegistry
        ? userTokens
            .filter((token) =>
              new Amount(
                getBalance(
                  selectedAccount.accountId,
                  token,
                  tokenBalancesRegistry
                )
              ).gt(0)
            )
            .filter(
              (token) => !token.isErc721 && !token.isErc1155 && !token.isNft
            )
            .map(getPriceIdForToken)
        : [],
    [selectedAccount, userTokens, tokenBalancesRegistry]
  )

  const { data: spotPriceRegistry, isLoading: isLoadingSpotPrices } =
    useGetTokenSpotPricesQuery(
      tokenPriceIds.length &&
        !isLoadingBalances &&
        !isFetchingBalances &&
        defaultFiatCurrency
        ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
        : skipToken,
      querySubscriptionOptions60s
    )

  const areTokenBalancesLoaded =
    selectedAccount &&
    tokenBalancesRegistry &&
    !isLoadingBalances &&
    !isFetchingBalances

  return (
    <StyledWrapper>
      <AddAssetButton onClick={onAddAsset}>
        {getLocale('braveWalletAddAsset')}
      </AddAssetButton>
      {userTokens?.map((token) => (
        <PortfolioAssetItem
          action={onClickAsset(
            token.contractAddress,
            token.symbol,
            token.tokenId,
            token.chainId
          )}
          key={getAssetIdKey(token)}
          assetBalance={
            areTokenBalancesLoaded
              ? getBalance(
                  selectedAccount.accountId,
                  token,
                  tokenBalancesRegistry
                )
              : ''
          }
          token={token}
          spotPrice={
            spotPriceRegistry && !isLoadingSpotPrices
              ? getTokenPriceAmountFromRegistry(
                  spotPriceRegistry,
                  token
                ).format()
              : ''
          }
          isPanel={true}
        />
      ))}
    </StyledWrapper>
  )
}

export default AssetsPanel
