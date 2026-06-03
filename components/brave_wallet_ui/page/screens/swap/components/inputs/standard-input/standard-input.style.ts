// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { StyledInput } from '../../shared-swap.styles'

export const Input = styled(StyledInput)`
  width: 100%;
  height: 32px;
  border: 1px solid ${leo.color.neutral[30]};
  border-radius: 4px;
  padding-left: 12px;
  font-weight: 200;
  ::placeholder {
    color: ${leo.color.text.tertiary};
    font-size: 14px;
    font-weight: 200;
  }
  :disabled {
    border: 1px solid ${leo.color.text.disabled};
    ::placeholder {
      color: ${leo.color.text.disabled};
    }
  }
`
