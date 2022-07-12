/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 import ITheme from 'brave-ui/theme/theme-interface'
 import defaultTheme from 'brave-ui/theme/brave-default'
 import IThemeShields from './theme'

 const lightTheme: ITheme & IThemeShields = {
   ...defaultTheme,
   name: 'Speedreader light',
   color: {
     ...defaultTheme.color,
     background01: '#FFFFFF'
   }
 }

 export default lightTheme
