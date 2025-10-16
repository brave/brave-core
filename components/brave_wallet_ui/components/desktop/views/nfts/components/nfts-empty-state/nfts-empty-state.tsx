// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import useMediaQuery from '$web-common/useMediaQuery'

// utils
import { getLocale } from '../../../../../../../common/locale'

// styles
import {
  DisclaimerText,
  EmptyStateImage,
  Heading,
  StyledWrapper,
  SubHeading,
  ImportButton,
} from './nfts-empty-state.style'
import EmptyStateGraphicLight from '../../../../../../assets/png-icons/nft-empty-state-light.png'
import EmptyStateGraphicDark from '../../../../../../assets/png-icons/nft-empty-state-dark.png'

interface Props {
  onImportNft: () => void
}

export const NftsEmptyState = ({ onImportNft }: Props) => {
  // hooks
  const isDarkMode = useMediaQuery('(prefers-color-scheme: dark)')

  return (
    <StyledWrapper>
      <EmptyStateImage
        src={isDarkMode ? EmptyStateGraphicDark : EmptyStateGraphicLight}
      />
      <Heading>{getLocale('braveNftsTabEmptyStateHeading')}</Heading>
      <SubHeading>{getLocale('braveNftsTabEmptyStateSubHeading')}</SubHeading>
      <ImportButton onClick={onImportNft}>
        {getLocale('braveNftsTabImportNft')}
      </ImportButton>
      <DisclaimerText>
        {getLocale('braveNftsTabEmptyStateDisclaimer')}
      </DisclaimerText>
    </StyledWrapper>
  )
}
