// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import NalaIcon, { IconProps } from '@brave/leo/react/icon'

type CustomIconType = React.ComponentType<
  React.PropsWithChildren<
    IconProps & {
      size?: string
      color?: string
    }
  >
>

export const Icon = styled<CustomIconType>(NalaIcon)`
  --leo-icon-size: ${(p) => p.size || 'unset'};
  --leo-icon-color: ${(p) => p.color || 'unset'};
`
