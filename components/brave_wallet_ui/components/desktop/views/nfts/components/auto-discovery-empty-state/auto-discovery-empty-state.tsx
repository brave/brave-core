// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// utils
import {
  getLocale,
  splitStringForTag
} from '../../../../../../../common/locale'

// styles
import {
  ActionButton,
  Description,
  Heading,
  LoadingRing,
  RefreshText,
  StyledWrapper
} from './auto-discovery-empty-state.styles'
import { Row, VerticalSpace } from '../../../../../shared/style'

interface Props {
  isRefreshingTokens: boolean
  onImportNft: () => void
  onRefresh: () => void
}

export const AutoDiscoveryEmptyState = ({
  isRefreshingTokens,
  onImportNft,
  onRefresh
}: Props) => {
  const { duringTag: refreshBtnText, afterTag } = splitStringForTag(
    getLocale('braveWalletAutoDiscoveryEmptyStateActions')
  )
  const { beforeTag: or, duringTag: importText } = splitStringForTag(
    afterTag || '',
    3
  )

  return (
    <StyledWrapper>
      {isRefreshingTokens ? (
        <>
          <LoadingRing />
          <VerticalSpace space='16px' />
          <RefreshText>
            {getLocale('braveWalletAutoDiscoveryEmptyStateRefresh')}...
          </RefreshText>
        </>
      ) : (
        <>
          <Heading>
            {getLocale('braveWalletAutoDiscoveryEmptyStateHeading')}
          </Heading>
          <Description>
            {getLocale('braveWalletAutoDiscoveryEmptyStateSubHeading')}
          </Description>
          <Row
            margin='48px 0 8px 0'
            marginBottom={8}
          >
            <Description>
              {getLocale('braveWalletAutoDiscoveryEmptyStateFooter')}
            </Description>
          </Row>
          <Description>
            <ActionButton onClick={onRefresh}>{refreshBtnText}</ActionButton>
            {or}
            <ActionButton onClick={onImportNft}>{importText}</ActionButton>
          </Description>
        </>
      )}
    </StyledWrapper>
  )
}
