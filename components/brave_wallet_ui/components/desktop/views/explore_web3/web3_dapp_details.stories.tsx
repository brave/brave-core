// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Route } from 'react-router'
import { WalletRoutes } from '../../../../constants/types'

import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { makeDappDetailsRoute } from '../../../../utils/routes-utils'
import { DappDetails } from './web3_dapp_details'

export const _Web3DappDetails = () => {
  return (
    <WalletPageStory
      initialRoute={makeDappDetailsRoute('3182') as WalletRoutes}
    >
      <Route path={WalletRoutes.Web3DappDetails} exact={false}>
        <DappDetails />
      </Route>
    </WalletPageStory>
  )
}

export default {}
