// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// utils
import { getLocale, formatLocale } from '$web-common/locale'

// styles
import {
  ActionButton,
  Heading,
  LoadingRing,
  StyledWrapper,
} from './auto-discovery-empty-state.styles'
import { Text, Row, VerticalSpace } from '../../../../../shared/style'

interface Props {
  isRefreshingTokens: boolean
  onImportNft: () => void
  onRefresh: () => void
}

export const AutoDiscoveryEmptyState = ({
  isRefreshingTokens,
  onImportNft,
  onRefresh,
}: Props) => {
  const emptyStateActions = formatLocale(
    'braveWalletAutoDiscoveryEmptyStateActions',
    {
      $1: (content) => (
        <ActionButton onClick={onRefresh}>{content}</ActionButton>
      ),
      $2: (content) => (
        <ActionButton onClick={onImportNft}>{content}</ActionButton>
      ),
    },
  )

  return (
    <StyledWrapper>
      {isRefreshingTokens ? (
        <>
          <LoadingRing />
          <VerticalSpace space='16px' />
          <Text
            textColor='secondary'
            variant='default.regular'
          >
            {getLocale('braveWalletAutoDiscoveryEmptyStateRefresh')}...
          </Text>
        </>
      ) : (
        <>
          <Heading>
            {getLocale('braveWalletAutoDiscoveryEmptyStateHeading')}
          </Heading>
          <Text
            textColor='tertiary'
            variant='small.regular'
          >
            {getLocale('braveWalletAutoDiscoveryEmptyStateSubHeading')}
          </Text>
          <Row
            margin='48px 0 8px 0'
            marginBottom={8}
          >
            <Text
              textColor='tertiary'
              variant='small.regular'
            >
              {getLocale('braveWalletAutoDiscoveryEmptyStateFooter')}
            </Text>
          </Row>
          <Text
            textColor='tertiary'
            variant='small.regular'
          >
            {emptyStateActions}
          </Text>
        </>
      )}
    </StyledWrapper>
  )
}
