/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import ITheme from 'brave-ui/theme/theme-interface'
import IThemeWalletPanel from './wallet-panel-theme'
import defaultTheme from 'brave-ui/theme/brave-default'
import colors from 'brave-ui/theme/colors'

const walletPanelDarkTheme: ITheme & IThemeWalletPanel = {
  ...defaultTheme,
  name: 'Wallet Panel Dark',
  color: {
    ...defaultTheme.color,
    text: colors.white,
    panelBackground: colors.grey900,
    outlineColor: 'rgba(255,255,255,0.5)'
  }
}

export default walletPanelDarkTheme
