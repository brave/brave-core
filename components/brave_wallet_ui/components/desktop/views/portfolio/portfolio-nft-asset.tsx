// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, useHistory, useParams } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import {
  BraveWallet,
  SupportedTestNetworks,
  UserAssetInfoType,
  WalletRoutes
} from '../../../../constants/types'

// Utils
import { getBalance } from '../../../../utils/balance-utils'
import Amount from '../../../../utils/amount'

// selectors
import { WalletSelectors } from '../../../../common/selectors'

// Options
import { AllNetworksOption } from '../../../../options/network-filter-options'

// Components
import { NftScreen } from '../../../../nft/components/nft-details/nft-screen'

// Hooks
import {
  useUnsafeWalletSelector
} from '../../../../common/hooks/use-safe-selector'
import {
  useGetNetworkQuery
} from '../../../../common/slices/api.slice'

// Styled Components
import { Skeleton } from '../../../shared/loading-skeleton/styles'
import { WalletPageWrapper } from '../../wallet-page-wrapper/wallet-page-wrapper'
import NftAssetHeader from '../../card-headers/nft-asset-header'
import { StyledWrapper } from './style'

export const PortfolioNftAsset = () => {
  // routing
  const history = useHistory()
  const { contractAddress, tokenId } = useParams<{
    contractAddress: string
    tokenId?: string
  }>()

  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const selectedNetworkFilter = useUnsafeWalletSelector(
    WalletSelectors.selectedNetworkFilter
  )

  // memos
  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useCallback(
    (asset: BraveWallet.BlockchainToken) => {
      const amounts = accounts
        .filter((account) => account.accountId.coin === asset.coin)
        .map((account) => getBalance(account, asset))

      // If a user has not yet created a FIL or SOL account,
      // we return 0 until they create an account
      if (amounts.length === 0) {
        return '0'
      }

      return amounts.reduce(function (a, b) {
        return a !== '' && b !== '' ? new Amount(a).plus(b).format() : ''
      })
    },
    [accounts]
  )

  // This looks at the users asset list and returns the full balance for each asset
  const userAssetList = React.useMemo(() => {
    const allAssets = userVisibleTokensInfo.map(
      (asset) =>
        ({
          asset: asset,
          assetBalance: fullAssetBalance(asset)
        } as UserAssetInfoType)
    )
    // By default we dont show any testnetwork assets
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return allAssets.filter(
        (asset) => !SupportedTestNetworks.includes(asset.asset.chainId)
      )
    }
    // If chainId is Localhost we also do a check for coinType to return
    // the correct asset
    if (selectedNetworkFilter.chainId === BraveWallet.LOCALHOST_CHAIN_ID) {
      return allAssets.filter(
        (asset) =>
          asset.asset.chainId === selectedNetworkFilter.chainId &&
          asset.asset.coin === selectedNetworkFilter.coin
      )
    }
    // Filter by all other assets by chainId's
    return allAssets.filter(
      (asset) => asset.asset.chainId === selectedNetworkFilter.chainId
    )
  }, [
    userVisibleTokensInfo,
    selectedNetworkFilter.chainId,
    selectedNetworkFilter.coin,
    fullAssetBalance
  ])

  // memos / computed
  const selectedAssetFromParams = React.useMemo(() => {
    const userToken = userVisibleTokensInfo.find((token) =>
      tokenId
        ? token.tokenId === tokenId &&
          token.contractAddress.toLowerCase() === contractAddress.toLowerCase()
        : token.contractAddress.toLowerCase() === contractAddress.toLowerCase()
    )
    return userToken
  }, [userVisibleTokensInfo, contractAddress, tokenId])

  const fullAssetBalances = React.useMemo(() => {
    if (selectedAssetFromParams?.contractAddress === '') {
      return userAssetList.find(
        (asset) =>
          asset.asset.symbol.toLowerCase() ===
            selectedAssetFromParams?.symbol.toLowerCase() &&
          asset.asset.chainId === selectedAssetFromParams?.chainId
      )
    }
    return userAssetList.find(
      (asset) =>
        asset.asset.contractAddress.toLowerCase() ===
          selectedAssetFromParams?.contractAddress.toLowerCase() &&
        asset.asset.chainId === selectedAssetFromParams?.chainId
    )
  }, [userAssetList, selectedAssetFromParams])

  // queries
  const { data: selectedAssetsNetwork } = useGetNetworkQuery(
    selectedAssetFromParams ?? skipToken //
  )

  const goBack = React.useCallback(() => {
    history.goBack()
  }, [userAssetList])

  const onSend = React.useCallback(() => {
    if (!selectedAssetFromParams || !selectedAssetsNetwork) return

    const account = accounts
      .filter(
        (account) => account.accountId.coin === selectedAssetFromParams.coin
      )
      .find((acc) =>
        new Amount(getBalance(acc, selectedAssetFromParams)).gte('1')
      )

    if (!account) return

    history.push(
      WalletRoutes.SendPage.replace(':chainId?', selectedAssetsNetwork.chainId)
        .replace(':accountAddress?', account.address)
        .replace(':contractAddress?', selectedAssetFromParams.contractAddress)
        .replace(':tokenId?', selectedAssetFromParams.tokenId)
    )
  }, [selectedAssetFromParams, accounts, selectedAssetsNetwork])

  // token list needs to load before we can find an asset to select from the url params
  if (userVisibleTokensInfo.length === 0) {
    return <Skeleton />
  }

  // asset not found
  if (!selectedAssetFromParams) {
    return <Redirect to={WalletRoutes.PortfolioNFTs} />
  }

  // render
  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      noCardPadding={false}
      hideDivider={false}
      cardHeader={
        <NftAssetHeader
          onBack={goBack}
          assetName={selectedAssetFromParams?.name}
          tokenId={selectedAssetFromParams?.tokenId}
          showSendButton={
            !new Amount(fullAssetBalances?.assetBalance || '').isZero()
          }
          onSend={onSend}
        />
      }
    >
      <StyledWrapper>
        {selectedAssetFromParams && (
          <NftScreen
            selectedAsset={selectedAssetFromParams}
            tokenNetwork={selectedAssetsNetwork}
          />
        )}
      </StyledWrapper>
    </WalletPageWrapper>
  )
}

export default PortfolioNftAsset
