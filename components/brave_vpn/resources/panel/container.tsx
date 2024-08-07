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
import { PanelWrapper } from './style'
import PurchaseFailedPanel from './components/purchase-failed-panel'

function Main() {
  const currentView = useSelector(state => state.currentView)
  const stateDescription = useSelector(state => state.stateDescription)

  if (currentView === ViewType.Loading) {
    return (
      <PanelWrapper>
        <LoadingPanel />
      </PanelWrapper>
    )
  }

  if (currentView === ViewType.Main) {
    return (
      <PanelWrapper>
        <MainPanel />
      </PanelWrapper>
    )
  }

  if (currentView === ViewType.PurchaseFailed) {
    return (
      <PanelWrapper>
        <PurchaseFailedPanel stateDescription={stateDescription} />
      </PanelWrapper>
    )
  }

  return (
    <PanelWrapper width={'360px'}>
      <SellPanel />
    </PanelWrapper>
  )
}

export default Main
