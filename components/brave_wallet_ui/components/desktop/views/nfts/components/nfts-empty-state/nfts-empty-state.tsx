// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// utils
import { getLocale } from '../../../../../../../common/locale'

// styles
import {
  DisclaimerText,
  EmptyStateImage,
  Heading,
  StyledWrapper,
  SubHeading,
  ImportButton
} from './nfts-empty-state.style'
import EmptyStateGraphic from '../../../../../../assets/png-icons/nft-empty-state.png'

interface Props {
  onImportNft: () => void
}

export const NftsEmptyState = ({ onImportNft }: Props) => (
  <StyledWrapper>
    <EmptyStateImage src={EmptyStateGraphic} />
    <Heading>{getLocale('braveNftsTabEmptyStateHeading')}</Heading>
    <SubHeading>{getLocale('braveNftsTabEmptyStateSubHeading')}</SubHeading>
    <ImportButton onClick={onImportNft}>{getLocale('braveNftsTabImportNft')}</ImportButton>
    <DisclaimerText>{getLocale('braveNftsTabEmptyStateDisclaimer')}</DisclaimerText>
  </StyledWrapper>
)
