// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import NalaButton, { ButtonProps } from '@brave/leo/react/button'

type CustomButtonType = React.ComponentType<
  React.PropsWithChildren<
    ButtonProps<undefined, boolean> & {
      padding?: string
      buttonColor?: string
    }
  >
>

export const Button = styled<CustomButtonType>(NalaButton)`
  --leo-button-padding: ${(p) => p.padding || 'unset'};
  --leo-button-color: ${(p) => p.buttonColor || 'unset'};
`
