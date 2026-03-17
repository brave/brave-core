// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import { App } from './components/app'
import * as BraveOriginMojom from 'gen/brave/components/brave_origin/mojom/brave_origin_startup.mojom.m.js'
import { loadTimeData } from '$web-common/loadTimeData'
import './strings'

setIconBasePath('//resources/brave-icons')

const handler = BraveOriginMojom.BraveOriginStartupHandler.getRemote()
const isLinuxFreeEligible = loadTimeData.getBoolean('isLinuxFreeEligible')

createRoot(document.getElementById('root')!).render(
  <App
    handler={handler}
    isLinuxFreeEligible={isLinuxFreeEligible}
  />,
)
