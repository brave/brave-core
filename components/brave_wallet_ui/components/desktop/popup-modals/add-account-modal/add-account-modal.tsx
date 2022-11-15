// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, Route, Switch } from 'react-router'

// types
import { WalletRoutes } from '../../../../constants/types'

// components
import { AddHardwareAccountModal } from './add-hardware-account-modal'
import { ImportAccountModal } from './add-imported-account-modal'
import { CreateAccountModal } from './create-account-modal'

export const AddAccountModal = () => {
  return (
    <Switch>

      <Route path={WalletRoutes.AddHardwareAccountModal} exact>
        <AddHardwareAccountModal />
      </Route>

      <Route path={WalletRoutes.ImportAccountModal} exact>
        <ImportAccountModal />
      </Route>

      <Route path={WalletRoutes.CreateAccountModal} exact>
        <CreateAccountModal />
      </Route>

      <Redirect to={WalletRoutes.CreateAccountModalStart} />

    </Switch>
  )
}
