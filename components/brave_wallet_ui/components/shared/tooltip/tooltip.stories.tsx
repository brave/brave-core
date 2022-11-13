// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { Tooltip } from '.'

export const _ToolTip = () => {
  return <Tooltip
    text='tip text'
  >
    Hover
  </Tooltip>
}

_ToolTip.storyName = 'Tooltip'

export default _ToolTip
