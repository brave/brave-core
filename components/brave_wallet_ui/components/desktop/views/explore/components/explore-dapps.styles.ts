// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

export const Title = styled.div`
  color: ${leo.color.text.primary};
  text-align: left;
  font-family: Poppins;
  font-size: 16px;
  font-style: normal;
  font-weight: 600;
`

export const SectionTitle = styled.div`
  color: ${leo.color.text.primary};
  text-align: left;
  font-family: Poppins;
  font-size: 18px;
  font-style: normal;
  font-weight: 500;
  line-height: 28px; /* 175% */
`
