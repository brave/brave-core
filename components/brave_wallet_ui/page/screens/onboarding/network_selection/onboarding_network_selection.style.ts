// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

export const GroupingText = styled.h3`
  color: ${leo.color.text.tertiary};
  font-family: 'Inter', 'Poppins';
  font-size: 12px;
  font-style: normal;
  font-weight: 600;
  line-height: 20px;
`

export const SelectAllText = styled.a`
  color: ${leo.color.text.interactive};
  font-family: 'Poppins';
  font-size: 12px;
  font-style: normal;
  font-weight: 600;
  line-height: 20px;
  letter-spacing: 0.36px;
  cursor: pointer;
`
