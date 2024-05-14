// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'

// Styled Components
import {
  StyledWrapper,
  Title,
  Description,
  ButtonWrapper,
  TokenName,
  ContractAddress,
  AssetIcon,
  TopWrapper,
  NetworkText,
  TopRow,
  LoadingRing
} from './style'
import { URLText } from '../shared-panel-styles'

// Components
import withPlaceholderIcon from '../../shared/create-placeholder-icon/index'
import { Tooltip } from '../../shared/tooltip/index'
import CreateSiteOrigin from '../../shared/create-site-origin/index'
import LoadingSkeleton from '../../shared/loading-skeleton/index'
import { NavButton } from '../buttons/nav-button/index'

// Utils
import { BraveWallet } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'

// Hooks
import useExplorer from '../../../common/hooks/explorer'
import {
  useApproveOrDeclineTokenSuggestionMutation,
  useGetNetworkQuery,
  useGetPendingTokenSuggestionRequestsQuery
} from '../../../common/slices/api.slice'

const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, {
  size: 'big',
  marginLeft: 0,
  marginRight: 0
})

export function AddSuggestedTokenPanel() {
  // queries
  const {
    data: suggestions = [],
    isFetching: isFetchingSuggestions,
    isLoading: isLoadingSuggestions
  } = useGetPendingTokenSuggestionRequestsQuery()
  const suggestion: BraveWallet.AddSuggestTokenRequest | undefined =
    suggestions[0]
  const { origin, token } = suggestion || {}

  const { data: tokenNetwork, isLoading: isFetchingTokenNetwork } =
    useGetNetworkQuery(token ?? skipToken)

  const isRefreshingSuggestions = isFetchingSuggestions || isLoadingSuggestions
  const isFetching = isRefreshingSuggestions || isFetchingTokenNetwork

  // mutations
  const [approveOrDeclineSuggestion] =
    useApproveOrDeclineTokenSuggestionMutation()

  // methods
  const onAddToken = React.useCallback(async () => {
    if (!token?.contractAddress || isRefreshingSuggestions) {
      return
    }
    await approveOrDeclineSuggestion({
      approved: true,
      contractAddress: token.contractAddress,
      closePanel: suggestions.length < 2
    })
  }, [
    approveOrDeclineSuggestion,
    token?.contractAddress,
    suggestions.length,
    isRefreshingSuggestions
  ])

  const onCancel = React.useCallback(async () => {
    if (!token?.contractAddress || isRefreshingSuggestions) {
      return
    }
    await approveOrDeclineSuggestion({
      approved: false,
      contractAddress: token.contractAddress,
      closePanel: suggestions.length < 2
    })
  }, [
    approveOrDeclineSuggestion,
    token?.contractAddress,
    suggestions.length,
    isRefreshingSuggestions
  ])

  // custom hooks
  const onClickViewOnBlockExplorer = useExplorer(tokenNetwork)

  // render
  return (
    <StyledWrapper>
      <TopWrapper>
        <TopRow>
          {!isFetching && tokenNetwork?.chainName ? (
            <NetworkText>{tokenNetwork.chainName}</NetworkText>
          ) : (
            <LoadingSkeleton
              height={12}
              width={'50px'}
            />
          )}
        </TopRow>
        <Title>{getLocale('braveWalletAddSuggestedTokenTitle')}</Title>
        <URLText>
          {isFetching || !origin ? (
            <LoadingSkeleton
              height={12}
              width={'50px'}
            />
          ) : (
            <CreateSiteOrigin
              originSpec={origin.originSpec}
              eTldPlusOne={origin.eTldPlusOne}
            />
          )}
        </URLText>
        <Description>
          {getLocale('braveWalletAddSuggestedTokenDescription')}
        </Description>
        {isFetching ? (
          <LoadingRing size={'40px'} />
        ) : (
          <AssetIconWithPlaceholder asset={token} />
        )}
        {!isFetching && token ? (
          <TokenName>
            {token.name ?? ''} ({token.symbol ?? ''})
          </TokenName>
        ) : (
          <LoadingSkeleton
            height={12}
            width={'50px'}
          />
        )}
        <Tooltip text={getLocale('braveWalletTransactionExplorer')}>
          <ContractAddress
            onClick={onClickViewOnBlockExplorer(
              'token',
              token?.contractAddress
            )}
          >
            {reduceAddress(token?.contractAddress ?? '')}
          </ContractAddress>
        </Tooltip>
      </TopWrapper>
      <ButtonWrapper>
        <NavButton
          disabled={isFetching}
          buttonType='secondary'
          text={getLocale('braveWalletButtonCancel')}
          onSubmit={onCancel}
        />
        <NavButton
          disabled={isFetching}
          buttonType='confirm'
          text={getLocale('braveWalletWatchListAdd')}
          onSubmit={onAddToken}
        />
      </ButtonWrapper>
    </StyledWrapper>
  )
}

export default AddSuggestedTokenPanel
