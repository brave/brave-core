// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Components
import {
  withPlaceholderIcon, //
} from '../../shared/create-placeholder-icon/index'
import { Tooltip } from '../../shared/tooltip/index'
import { Skeleton } from '../../shared/loading-skeleton/styles'
import { OriginInfoCard } from '../origin_info_card/origin_info_card'

// Utils
import { BraveWallet } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'

// Hooks
import useExplorer from '../../../common/hooks/explorer'
import {
  useApproveOrDeclineTokenSuggestionMutation,
  useGetNetworkQuery,
  useGetPendingTokenSuggestionRequestsQuery,
} from '../../../common/slices/api.slice'

// Styled Components
import {
  StyledWrapper,
  Description,
  TokenName,
  ContractAddress,
  AssetIcon,
  HeaderText,
  Card,
  TokenDescription,
} from './add_suggested_token_panel.style'
import { Column, Row, VerticalDivider } from '../../shared/style'

const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, {
  size: 'big',
  marginLeft: 0,
  marginRight: 0,
})

export function AddSuggestedTokenPanel() {
  // Queries
  const {
    data: suggestions = [],
    isFetching: isFetchingSuggestions,
    isLoading: isLoadingSuggestions,
  } = useGetPendingTokenSuggestionRequestsQuery()
  const suggestion: BraveWallet.AddSuggestTokenRequest | undefined =
    suggestions[0]
  const { origin, token } = suggestion || {}

  const { data: tokensNetwork, isLoading: isFetchingTokensNetwork } =
    useGetNetworkQuery(token ?? skipToken)

  // Computed
  const isRefreshingSuggestions = isFetchingSuggestions || isLoadingSuggestions
  const isFetching = isRefreshingSuggestions || isFetchingTokensNetwork

  // Mutations
  const [approveOrDeclineSuggestion] =
    useApproveOrDeclineTokenSuggestionMutation()

  // Methods
  const onAddToken = React.useCallback(async () => {
    if (!token?.contractAddress || isRefreshingSuggestions) {
      return
    }
    await approveOrDeclineSuggestion({
      approved: true,
      contractAddress: token.contractAddress,
      closePanel: suggestions.length < 2,
    })
  }, [
    approveOrDeclineSuggestion,
    token?.contractAddress,
    suggestions.length,
    isRefreshingSuggestions,
  ])

  const onCancel = React.useCallback(async () => {
    if (!token?.contractAddress || isRefreshingSuggestions) {
      return
    }
    await approveOrDeclineSuggestion({
      approved: false,
      contractAddress: token.contractAddress,
      closePanel: suggestions.length < 2,
    })
  }, [
    approveOrDeclineSuggestion,
    token?.contractAddress,
    suggestions.length,
    isRefreshingSuggestions,
  ])

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(tokensNetwork)

  // render
  return (
    <StyledWrapper
      width='100%'
      height='100%'
      justifyContent='space-between'
    >
      <Row padding='18px'>
        <HeaderText textColor='primary'>
          {getLocale('braveWalletAddSuggestedTokenTitle')}
        </HeaderText>
      </Row>
      <VerticalDivider />
      <Column
        padding='8px'
        width='100%'
        height='100%'
      >
        <Card
          width='100%'
          height='100%'
          justifyContent='space-between'
        >
          {origin && <OriginInfoCard origin={origin} />}
          <Column
            width='100%'
            height='100%'
            padding='32px 16px 16px 16px'
            gap='56px'
            justifyContent='flex-start'
          >
            <Description textColor='primary'>
              {getLocale('braveWalletAddSuggestedTokenDescription')}
            </Description>
            <Column>
              {isFetching ? (
                <Skeleton
                  width={80}
                  height={80}
                  enableAnimation={true}
                  circle={true}
                />
              ) : (
                <AssetIconWithPlaceholder asset={token} />
              )}
              {!isFetching && token ? (
                <>
                  <TokenName textColor='tertiary'>{token.name ?? ''}</TokenName>
                  <TokenDescription textColor='primary'>
                    {getLocale('braveWalletPortfolioAssetNetworkDescription')
                      .replace('$1', token?.symbol ?? '')
                      .replace('$2', tokensNetwork?.chainName ?? '')}
                  </TokenDescription>
                  <Tooltip text={getLocale('braveWalletTransactionExplorer')}>
                    <ContractAddress
                      onClick={onClickViewOnBlockExplorer(
                        'token',
                        token?.contractAddress,
                      )}
                    >
                      {reduceAddress(token?.contractAddress ?? '')}
                    </ContractAddress>
                  </Tooltip>
                </>
              ) : (
                <Column
                  gap='8px'
                  padding='16px 0px 0px 0px'
                >
                  <Skeleton
                    enableAnimation={true}
                    height={16}
                    width={100}
                  />
                  <Skeleton
                    enableAnimation={true}
                    height={16}
                    width={140}
                  />
                  <Skeleton
                    enableAnimation={true}
                    height={14}
                    width={100}
                  />
                </Column>
              )}
            </Column>
          </Column>
          <Column
            width='100%'
            gap='16px'
            padding='0px 16px 36px 16px'
          >
            <Row>
              <Button
                onClick={onAddToken}
                size='medium'
                kind='filled'
                disabled={isFetching}
              >
                <Icon
                  slot='icon-before'
                  name='check-normal'
                />
                {getLocale('braveWalletAddToken')}
              </Button>
            </Row>
            <Row>
              <Button
                onClick={onCancel}
                size='medium'
                kind='outline'
                disabled={isFetching}
              >
                {getLocale('braveWalletButtonCancel')}
              </Button>
            </Row>
          </Column>
        </Card>
      </Column>
    </StyledWrapper>
  )
}

export default AddSuggestedTokenPanel
