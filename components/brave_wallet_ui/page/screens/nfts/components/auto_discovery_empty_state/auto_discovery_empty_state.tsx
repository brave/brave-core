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
} from './auto_discovery_empty_state.styles'
import {
  Text,
  Row,
  VerticalSpace,
} from '../../../../../components/shared/style'

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
    S.BRAVE_WALLET_AUTO_DISCOVERY_EMPTY_STATE_ACTIONS,
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
            {getLocale(S.BRAVE_WALLET_AUTO_DISCOVERY_EMPTY_STATE_REFRESH)}...
          </Text>
        </>
      ) : (
        <>
          <Heading>
            {getLocale(S.BRAVE_WALLET_AUTO_DISCOVERY_EMPTY_STATE_HEADING)}
          </Heading>
          <Text
            textColor='tertiary'
            variant='small.regular'
          >
            {getLocale(S.BRAVE_WALLET_AUTO_DISCOVERY_EMPTY_STATE_SUB_HEADING)}
          </Text>
          <Row
            margin='48px 0 8px 0'
            marginBottom={8}
          >
            <Text
              textColor='tertiary'
              variant='small.regular'
            >
              {getLocale(S.BRAVE_WALLET_AUTO_DISCOVERY_EMPTY_STATE_FOOTER)}
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
