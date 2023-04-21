// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import MainPanel from './components/main-panel'
import SellPanel from './components/sell-panel'
import LoadingPanel from './components/loading-panel'
import { ViewType } from './state/component_types'
import { useSelector } from './state/hooks'
import PurchaseFailedPanel from './components/purchase-failed-panel'

function Main () {
  const currentView = useSelector(state => state.currentView)
  const stateDescription = useSelector(state => state.stateDescription)

  if (currentView === ViewType.Loading) {
    return (
      <LoadingPanel />
    )
  }

  if (currentView === ViewType.Main) {
    return (
      <MainPanel />
    )
  }

  if (currentView === ViewType.PurchaseFailed) {
    return (
      <PurchaseFailedPanel stateDescription={stateDescription} />
    )
  }

  return (
    <SellPanel />
  )
}

export default Main
