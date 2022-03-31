/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 import ITheme from 'brave-ui/theme/theme-interface'
 import defaultDarkTheme from 'brave-ui/theme/brave-dark'

 const shieldsDarkTheme: ITheme = {
   ...defaultDarkTheme,
   name: 'Shields Dark',
   color: {
     ...defaultDarkTheme.color,
     background03: '#3B3E4F',
     text01: '#F0F2FF'
   }
 }

 export default shieldsDarkTheme
