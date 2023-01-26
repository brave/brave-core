// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'

// Components
import { App } from './components/app'

function initialize () {
  render(
    <App />,
    document.getElementById('root'))
}

document.addEventListener('DOMContentLoaded', initialize)
