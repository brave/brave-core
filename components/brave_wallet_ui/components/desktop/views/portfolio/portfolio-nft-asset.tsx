// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, useHistory, useParams } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import {
  SendPageTabHashes,
  WalletRoutes
} from '../../../../constants/types'

// Utils
import { getBalance } from '../../../../utils/balance-utils'
import Amount from '../../../../utils/amount'

// selectors
import { WalletSelectors } from '../../../../common/selectors'

// Components
import { NftScreen } from '../../../../nft/components/nft-details/nft-screen'

// Hooks
import {
  useUnsafeWalletSelector
} from '../../../../common/hooks/use-safe-selector'
import {
  useGetNetworkQuery
} from '../../../../common/slices/api.slice'
import {
  useScopedBalanceUpdater
} from '../../../../common/hooks/use-scoped-balance-updater'

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
  const hiddenNfts =
    useUnsafeWalletSelector(WalletSelectors.removedNonFungibleTokens)

  const selectedAssetFromParams = React.useMemo(() => {
    const userToken =
      [...userVisibleTokensInfo, ...hiddenNfts]
        .find((token) =>
          tokenId
            ? token.tokenId === tokenId &&
            token.contractAddress.toLowerCase() ===
            contractAddress.toLowerCase()
            : token.contractAddress.toLowerCase() ===
            contractAddress.toLowerCase()
        )
    return userToken
  }, [
    userVisibleTokensInfo,
    contractAddress,
    tokenId,
    hiddenNfts
  ])

  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const { data: selectedAssetNetwork } = useGetNetworkQuery(
    selectedAssetFromParams ?? skipToken
  )

  const candidateAccounts = React.useMemo(() => {
    if (!selectedAssetFromParams) {
      return []
    }

    return accounts.filter((account) =>
      account.accountId.coin === selectedAssetFromParams.coin)
  }, [accounts, selectedAssetFromParams])

  const {
    data: tokenBalancesRegistry,
  } = useScopedBalanceUpdater(
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

    return candidateAccounts.find(account =>
      new Amount(
        getBalance(
          account.accountId,
          selectedAssetFromParams,
          tokenBalancesRegistry
        )
      ).gt(0))
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
    if (!selectedAssetFromParams || !selectedAssetNetwork || !ownerAccount) {
      return
    }

    history.push(
      `${WalletRoutes.SendPage
        .replace(':chainId?', selectedAssetNetwork.chainId)
        .replace(':accountAddress?', ownerAccount.address)
        .replace(
          ':contractAddressOrSymbol?',
          selectedAssetFromParams.contractAddress
        )
        .replace(':tokenId?', selectedAssetFromParams.tokenId)}${ //
      SendPageTabHashes.nft}`
    )
  }, [selectedAssetFromParams, ownerAccount, selectedAssetNetwork])

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
          onBack={history.goBack}
          assetName={selectedAssetFromParams?.name}
          tokenId={selectedAssetFromParams?.tokenId}
          showSendButton={showSendButton}
          onSend={onSend}
        />
      }
    >
      <StyledWrapper>
        {selectedAssetFromParams && (
          <NftScreen
            selectedAsset={selectedAssetFromParams}
            tokenNetwork={selectedAssetNetwork}
          />
        )}
      </StyledWrapper>
    </WalletPageWrapper>
  )
}

export default PortfolioNftAsset
