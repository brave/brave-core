// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Route } from 'react-router'

// routes
import { WalletRoutes } from '../../../../constants/types'

// utils
import { makeDappDetailsRoute } from '../../../../utils/routes-utils'
import { getLocale } from '../../../../../common/locale'

// components
import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { DappDetails } from './web3_dapp_details'
import WalletPageWrapper from '../../wallet-page-wrapper/wallet-page-wrapper'
import { PageTitleHeader } from '../../card-headers/page-title-header'

export const _Web3DappDetails = () => {
  return (
    <WalletPageStory
      initialRoute={makeDappDetailsRoute('3182') as WalletRoutes}
    >
      <Route
        path={WalletRoutes.Web3DappDetails}
        exact={false}
      >
        <WalletPageWrapper
          wrapContentInBox
          cardHeader={
            <PageTitleHeader
              title={getLocale('braveWalletAccountSettingsDetails')}
            />
          }
        >
          <DappDetails />
        </WalletPageWrapper>
      </Route>
    </WalletPageStory>
  )
}

export default {}
