// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import IBraveUITheme from 'brave-ui/theme/theme-interface'
import LeoTheme from '@brave/leo/tokens/styledComponents/theme'

type LeoTheme = typeof LeoTheme

declare module 'styled-components' {
  export interface DefaultTheme extends LeoTheme {
    // TODO(petemill): Remove usage of IBraveUITheme in favor of Leo tokens
    legacy: IBraveUITheme
  }
}
