// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, useHistory, useParams } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import { WalletRoutes } from '../../../../constants/types'

// Utils
import { getBalance } from '../../../../utils/balance-utils'
import { makeSendRoute } from '../../../../utils/routes-utils'
import Amount from '../../../../utils/amount'
import { getAssetIdKey } from '../../../../utils/asset-utils'

// Components
import { NftScreen } from '../../../../nft/components/nft-details/nft-screen'

// Hooks
import {
  useGetNetworkQuery,
  useGetSimpleHashSpamNftsQuery,
  useGetUserTokensRegistryQuery
} from '../../../../common/slices/api.slice'
import {
  useScopedBalanceUpdater //
} from '../../../../common/hooks/use-scoped-balance-updater'
import { useAccountsQuery } from '../../../../common/slices/api.slice.extra'
import {
  selectAllVisibleUserAssetsFromQueryResult, //
  selectHiddenNftsFromQueryResult
} from '../../../../common/slices/entities/blockchain-token.entity'

// Styled Components
import { Skeleton } from '../../../shared/loading-skeleton/styles'
import { WalletPageWrapper } from '../../wallet-page-wrapper/wallet-page-wrapper'
import NftAssetHeader from '../../card-headers/nft-asset-header'
import { StyledWrapper } from './style'

export const PortfolioNftAsset = () => {
  // routing
  const history = useHistory()
  const { assetId } = useParams<{
    assetId?: string
  }>()

  // queries
  const { data: simpleHashNfts = [], isLoading: isLoadingSpamNfts } =
    useGetSimpleHashSpamNftsQuery()

  const { hiddenNfts, userVisibleTokensInfo, isLoadingTokens } =
    useGetUserTokensRegistryQuery(undefined, {
      selectFromResult: (result) => ({
        hiddenNfts: selectHiddenNftsFromQueryResult(result),
        userVisibleTokensInfo:
          selectAllVisibleUserAssetsFromQueryResult(result),
        isLoadingTokens: result.isLoading
      })
    })

  const selectedAssetFromParams = React.useMemo(() => {
    return userVisibleTokensInfo
      .concat(hiddenNfts)
      .concat(simpleHashNfts)
      .find((token) => {
        return getAssetIdKey(token) === assetId
      })
  }, [assetId, userVisibleTokensInfo, hiddenNfts, simpleHashNfts])

  const { accounts } = useAccountsQuery()
  const { data: selectedAssetNetwork } = useGetNetworkQuery(
    selectedAssetFromParams ?? skipToken
  )

  const candidateAccounts = React.useMemo(() => {
    if (!selectedAssetFromParams) {
      return []
    }

    return accounts.filter(
      (account) => account.accountId.coin === selectedAssetFromParams.coin
    )
  }, [accounts, selectedAssetFromParams])

  const { data: tokenBalancesRegistry } = useScopedBalanceUpdater(
    selectedAssetFromParams && candidateAccounts && selectedAssetNetwork
      ? {
          network: selectedAssetNetwork,
          accounts: candidateAccounts,
          tokens: [selectedAssetFromParams]
        }
      : skipToken
  )

  const ownerAccount = React.useMemo(() => {
    if (!candidateAccounts) return

    return candidateAccounts.find((account) =>
      new Amount(
        getBalance(
          account.accountId,
          selectedAssetFromParams,
          tokenBalancesRegistry
        )
      ).gt(0)
    )
  }, [selectedAssetFromParams, candidateAccounts, tokenBalancesRegistry])

  const showSendButton = React.useMemo(() => {
    if (!selectedAssetFromParams || !ownerAccount) return false

    const balance = getBalance(
      ownerAccount.accountId,
      selectedAssetFromParams,
      tokenBalancesRegistry
    )
    return new Amount(balance).gt('0')
  }, [selectedAssetFromParams, ownerAccount, tokenBalancesRegistry])

  const onSend = React.useCallback(() => {
    if (!selectedAssetFromParams || !ownerAccount) {
      return
    }

    history.push(makeSendRoute(selectedAssetFromParams, ownerAccount))
  }, [selectedAssetFromParams, ownerAccount, history])

  // asset not found
  if (
    !selectedAssetFromParams &&
    !isLoadingSpamNfts &&
    !isLoadingTokens &&
    userVisibleTokensInfo.length === 0
  ) {
    return <Redirect to={WalletRoutes.PortfolioNFTs} />
  }

  // render
  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      noCardPadding={false}
      cardHeader={
        <NftAssetHeader
          onBack={history.goBack}
          assetName={selectedAssetFromParams?.name}
          tokenId={selectedAssetFromParams?.tokenId}
          onSend={showSendButton ? onSend : undefined}
        />
      }
    >
      <StyledWrapper>
        {selectedAssetFromParams ? (
          <NftScreen
            selectedAsset={selectedAssetFromParams}
            tokenNetwork={selectedAssetNetwork}
          />
        ) : (
          <Skeleton />
        )}
      </StyledWrapper>
    </WalletPageWrapper>
  )
}

export default PortfolioNftAsset
