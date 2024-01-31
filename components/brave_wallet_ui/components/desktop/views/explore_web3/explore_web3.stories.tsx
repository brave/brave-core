// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../../common/locale'

import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { PageTitleHeader } from '../../card-headers/page-title-header'
import WalletPageWrapper from '../../wallet-page-wrapper/wallet-page-wrapper'
import { StyledWrapper } from '../crypto/style'
import { ExploreWeb3View } from './explore_web3'

export const _ExploreWeb3View = () => {
  return (
    <WalletPageStory>
      <WalletPageWrapper
        wrapContentInBox
        cardHeader={
          <PageTitleHeader title={getLocale('braveWalletTopNavExplore')} />
        }
      >
        <StyledWrapper>
          <ExploreWeb3View />
        </StyledWrapper>
      </WalletPageWrapper>
    </WalletPageStory>
  )
}

export default {}
