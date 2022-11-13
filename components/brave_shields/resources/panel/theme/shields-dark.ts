/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import ITheme from 'brave-ui/theme/theme-interface'
 import defaultDarkTheme from 'brave-ui/theme/brave-dark'
 import IThemeShields from './shields-theme'

 const shieldsDarkTheme: ITheme & IThemeShields = {
   ...defaultDarkTheme,
   name: 'Shields Dark',
   color: {
     ...defaultDarkTheme.color,
     background03: '#313341',
     text01: '#F0F2FF',
     interactive05: '#737ADE',
     interactive06: '#A0A5EB',
     interactive07: '#737ADE',
     interactive08: '#4C54D2'
   }
 }

 export default shieldsDarkTheme
