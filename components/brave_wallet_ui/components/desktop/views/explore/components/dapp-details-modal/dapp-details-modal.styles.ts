// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

export const Name = styled.div`
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 28px;
  font-style: normal;
  font-weight: 500;
  line-height: 40px;
  text-align: center;
`

export const Category = styled.div`
  display: flex;
  height: 20px;
  padding: 0px ${leo.spacing.s};
  align-items: center;
  border-radius: ${leo.spacing.s};
  background: ${leo.color.gray['20']};
  color: ${leo.color.gray['50']};
  font-family: Poppins;
  font-size: 10px;
  font-style: normal;
  font-weight: 600;
  line-height: normal;
  text-transform: uppercase;
`

export const Description = styled.div`
  color: ${leo.color.text.primary};
  text-align: center;
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 400;
  line-height: 24px;
`
