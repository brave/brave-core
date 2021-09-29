/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import ITheme from 'brave-ui/theme/theme-interface'
import IThemeVPN from './vpn-theme'
import defaultDarkTheme from 'brave-ui/theme/brave-dark'

const vpnDarkTheme: ITheme & IThemeVPN = {
  ...defaultDarkTheme,
  name: 'VPN Dark',
  color: {
    ...defaultDarkTheme.color,
    panelBackground: 'linear-gradient(180deg, #1E2029 59.48%, #332C60 100%)'
  }
}

export default vpnDarkTheme
