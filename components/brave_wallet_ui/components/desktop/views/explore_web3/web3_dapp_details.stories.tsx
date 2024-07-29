// Copyright (c) 2023 The Brave Authors. All rights reserved.
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
import { DappDetails } from './web3_dapp_details'
import WalletPageWrapper from '../../wallet-page-wrapper/wallet-page-wrapper'
import { PageTitleHeader } from '../../card-headers/page-title-header'
import { useGetTopDappsQuery } from '../../../../common/slices/api.slice'

export const _DappDetails = () => {
  const { data: topDapps } = useGetTopDappsQuery()

  return (
    <WalletPageStory>
      <WalletPageWrapper
        wrapContentInBox
        cardHeader={<PageTitleHeader title={getLocale('braveWalletDetails')} />}
      >
        {topDapps && <DappDetails dapp={topDapps[0]} />}
      </WalletPageWrapper>
    </WalletPageStory>
  )
}

export default { component: _DappDetails }
