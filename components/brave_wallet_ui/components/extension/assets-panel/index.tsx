// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

import {
  WalletRoutes,
  WalletAccountType,
  BraveWallet
} from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'
import { getAssetIdKey } from '../../../utils/asset-utils'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { getTokenPriceAmountFromRegistry } from '../../../utils/pricing-utils'
import Amount from '../../../utils/amount'

// Styled Components
import { StyledWrapper, AddAssetButton } from './style'

// RTK
import {
  useGetSelectedChainQuery,
  useGetTokenSpotPricesQuery,
  useGetUserTokensRegistryQuery
} from '../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../common/slices/constants'
import {
  makeSelectAllUserAssetsForChainFromQueryResult
} from '../../../common/slices/entities/blockchain-token.entity'

// Hooks
import {
  useScopedBalanceUpdater
} from '../../../common/hooks/use-scoped-balance-updater'

import { PortfolioAssetItem } from '../../desktop'

interface Props {
  selectedAccount?: WalletAccountType
  onAddAsset: () => void
}

const AssetsPanel = (props: Props) => {
  const { selectedAccount, onAddAsset } = props

  const routeToAssetDetails = (url: string) => {
    chrome.tabs.create({ url: url }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const { currentData: selectedNetwork } = useGetSelectedChainQuery(undefined)

  const userTokensByChainIdSelector = React.useMemo(() => {
    return makeSelectAllUserAssetsForChainFromQueryResult()
  }, [])

  const { data: userTokens } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      ...res,
      data:
        selectedNetwork !== undefined
          ? userTokensByChainIdSelector(res, selectedNetwork.chainId)
          : undefined
    }),
    skip: selectedNetwork === undefined
  })

  const {
    data: balances,
    isLoading: isLoadingBalances,
    isFetching: isFetchingBalances
  } = useScopedBalanceUpdater(
    selectedNetwork && selectedAccount
      ? {
          network: selectedNetwork,
          account: selectedAccount,
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

  const {
    data: spotPriceRegistry,
    isLoading: isLoadingSpotPrices,
    isFetching: isFetchingSpotPrices
  } = useGetTokenSpotPricesQuery(
    userTokens && balances && !isLoadingBalances && !isFetchingBalances
      ? {
        ids: userTokens
          .filter(token => new Amount(balances[token.contractAddress]).gt(0))
          .filter(token => !token.isErc721 && !token.isErc1155 && !token.isNft)
          .map(getPriceIdForToken)
      }
      : skipToken,
    querySubscriptionOptions60s
  )

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
            balances && !isLoadingBalances && !isFetchingBalances
              ? balances[token.contractAddress] ?? '0'
              : ''
          }
          token={token}
          spotPrice={
            spotPriceRegistry && !isLoadingSpotPrices && !isFetchingSpotPrices
              ? getTokenPriceAmountFromRegistry(spotPriceRegistry, token)
              : Amount.empty()
          }
          isPanel={true}
        />
      ))}
    </StyledWrapper>
  )
}

export default AssetsPanel
