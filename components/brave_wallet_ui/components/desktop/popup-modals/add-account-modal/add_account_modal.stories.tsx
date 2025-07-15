// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Route, Switch, Redirect } from 'react-router'

// types
import { WalletRoutes } from '../../../../constants/types'

import {
  WalletPageStory, //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { AddAccountModal } from './add-account-modal'

export const _CreateAccountModal = {
  render: () => {
    return (
      <WalletPageStory>
        <AddAccountModal />
      </WalletPageStory>
    )
  },
}

export const _AddHardwareAccountModal = {
  render: () => {
    return (
      <WalletPageStory>
        <Switch>
          <Route path={WalletRoutes.AddHardwareAccountModal}>
            <AddAccountModal />
          </Route>
          <Redirect to={WalletRoutes.AddHardwareAccountModalStart} />
        </Switch>
      </WalletPageStory>
    )
  },
}

export const _ImportAccountModal = {
  render: () => {
    return (
      <WalletPageStory>
        <Switch>
          <Route path={WalletRoutes.ImportAccountModal}>
            <AddAccountModal />
          </Route>
          <Redirect to={WalletRoutes.ImportAccountModalStart} />
        </Switch>
      </WalletPageStory>
    )
  },
}

export default {
  title: 'Wallet/Desktop/Components/Popup Modals/Add Account',
  component: AddAccountModal,
}
