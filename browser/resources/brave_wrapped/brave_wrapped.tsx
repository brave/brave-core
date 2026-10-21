// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import StyledComponentsProvider from '$web-common/StyledComponentsProvider'
import { loadTimeData } from '$web-common/loadTimeData'
import { App } from './components/app'
import { WrappedStats } from './stats'
import './strings'

setIconBasePath('chrome://resources/brave-icons')

const stats: WrappedStats = {
  adsBlocked: Number(loadTimeData.getString('adsBlocked')),
  bandwidthSavedBytes: Number(loadTimeData.getString('bandwidthSavedBytes')),
  firstRunTime: Number(loadTimeData.getString('firstRunTime')),
}

createRoot(document.getElementById('root')!).render(
  <StyledComponentsProvider>
    <App stats={stats} />
  </StyledComponentsProvider>,
)
