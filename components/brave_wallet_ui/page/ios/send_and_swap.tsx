// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Route, Switch } from 'react-router-dom'

// utils
import { getWalletLocationTitle } from '../../utils/string-utils'

// style
import 'emptykit.css'

// Components
import { SendScreen } from '../screens/send/send_screen/send_screen'
import { Swap } from '../screens/swap/swap'

// types
import { WalletRoutes } from '../../constants/types'

// hooks
import { useLocationPathName } from '../../common/hooks/use-pathname'

import { setIconBasePath } from '@brave/leo/react/icon'
setIconBasePath('chrome://resources/brave-icons')

export const IOSSendSwap = () => {
  const walletLocation = useLocationPathName()
  React.useEffect(() => {
    // update page title
    document.title = getWalletLocationTitle(walletLocation)
  }, [walletLocation])
  return (
    <Switch>
    <Route path={WalletRoutes.Send}>
      <SendScreen isIOS={true} />
    </Route>
    <Route path={WalletRoutes.Swap}>
      <Swap isIOS={true} />
    </Route>
  </Switch>
  )
}

export default IOSSendSwap
