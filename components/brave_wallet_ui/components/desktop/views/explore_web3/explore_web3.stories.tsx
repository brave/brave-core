// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'

// components
import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import WalletPageWrapper from '../../wallet-page-wrapper/wallet-page-wrapper'
import { PageTitleHeader } from '../../card-headers/page-title-header'
import { ExploreWeb3View } from './explore_web3'

// styles
import { StyledWrapper } from '../crypto/style'

export const _ExploreWeb3View = {
  render: () => {
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
}

export default { component: ExploreWeb3View }
