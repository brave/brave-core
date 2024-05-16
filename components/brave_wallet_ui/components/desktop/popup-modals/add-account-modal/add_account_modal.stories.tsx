// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Route, Switch, Redirect } from 'react-router'

// types
import { WalletRoutes } from '../../../../constants/types'

import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { AddAccountModal } from './add-account-modal'

export const _CreateAccountModal = () => {
  return (
    <WalletPageStory>
      <AddAccountModal />
    </WalletPageStory>
  )
}

_CreateAccountModal.storyName = 'Create Account Modal'

export const _AddHardwareAccountModal = () => {
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
}

_AddHardwareAccountModal.storyName = 'Add Hardware Account Modal'

export const _ImportAccountModal = () => {
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
}

_ImportAccountModal.storyName = 'Import Account Modal'

export default _CreateAccountModal
