/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import ITheme from 'brave-ui/theme/theme-interface'
import IThemeVPN from './vpn-theme'
import defaultTheme from 'brave-ui/theme/brave-default'

const vpnLightTheme: ITheme & IThemeVPN = {
  ...defaultTheme,
  name: 'VPN Light',
  color: {
    ...defaultTheme.color,
    panelBackground: 'linear-gradient(180deg, #FFFFFF 58.56%, #F8F8FF 100%)'
  }
}

export default vpnLightTheme
