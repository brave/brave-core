/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import ITheme from 'brave-ui/theme/theme-interface'
import IThemeWallet from './wallet-theme'
import defaultTheme from 'brave-ui/theme/brave-default'
import colors from 'brave-ui/theme/colors'

const walletLightTheme: ITheme & IThemeWallet = {
  ...defaultTheme,
  name: 'Wallet Light',
  color: {
    ...defaultTheme.color,
    text: colors.neutral900,
    panelBackground: '#F9F9FD',
    panelBackgroundSecondary: colors.neutral000,
    outlineColor: 'rgba(255,80,0,0.2)'
  }
}

export default walletLightTheme
