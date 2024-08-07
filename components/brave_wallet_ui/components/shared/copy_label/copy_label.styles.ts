// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { walletButtonFocusMixin } from '../style'

export const LabelButton = styled.button`
  padding: 0;
  border: none;
  cursor: pointer;
  background: none;
  ${walletButtonFocusMixin}
`

export const LabelText = styled.span`
  text-transform: none;
`
