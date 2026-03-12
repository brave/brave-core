/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { setIconBasePath } from '@brave/leo/react/icon'

setIconBasePath('chrome://resources/brave-icons')

// Containers
import PsstDlgContainer from './containers/App'

function initialize() {
  render(
    <PsstDlgContainer/>,
    document.getElementById('root'),
  )
}

document.addEventListener('DOMContentLoaded', initialize)
