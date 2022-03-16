/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 import ITheme from 'brave-ui/theme/theme-interface'
 import defaultTheme from 'brave-ui/theme/brave-default'

 const shieldsLightTheme: ITheme = {
   ...defaultTheme,
   name: 'Shields light',
   color: {
     ...defaultTheme.color,
     background03: '#F8F9FA',
     text01: '#212529',
     warningIcon: '#EA3A0D'
   }
 }

 export default shieldsLightTheme
