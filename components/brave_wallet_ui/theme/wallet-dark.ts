/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import ITheme from 'brave-ui/theme/theme-interface'
import IThemeWallet from './wallet-theme'
import defaultDarkTheme from 'brave-ui/theme/brave-dark'
import colors from 'brave-ui/theme/colors'

const walletDarkTheme: ITheme & IThemeWallet = {
  ...defaultDarkTheme,
  name: 'Wallet Dark',
  color: {
    ...defaultDarkTheme.color,
    text: colors.white,
    panelBackground: colors.grey900,
    outlineColor: 'rgba(255,255,255,0.5)'
  }
}

export default walletDarkTheme
