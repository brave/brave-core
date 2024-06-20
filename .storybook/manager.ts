// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { addons } from '@storybook/manager-api'
import { create } from '@storybook/theming'

const braveTheme = create({
  base: 'dark',
  brandTitle: 'Brave Browser UI',
  brandUrl: 'https://github.com/brave/brave-core'
})

addons.setConfig({
  isFullscreen: false,
  showNav: true,
  showPanel: true,
  panelPosition: 'right',
  theme: braveTheme
})
