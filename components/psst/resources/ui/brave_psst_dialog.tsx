/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

// Containers
import PsstDlgContainer from './containers/App'

function initialize() {
  setIconBasePath('//resources/brave-icons')
  createRoot(document.getElementById('root')!).render(<PsstDlgContainer />)
}

document.addEventListener('DOMContentLoaded', initialize)
