// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import IBraveTheme from 'brave-ui/theme/theme-interface'

export default function customizeTheme (theme: IBraveTheme): IBraveTheme {
  return {
    ...theme,
    color: {
      ...theme.color,
      primaryBackground: 'rgba(2, 166, 194, 1)',
      secondaryBackground: 'rgba(33, 46, 60, 1)'
    }
  }
}
