/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

export const title = styled.div`
  display: flex;
  align-items: center;
  gap: 8px;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const checkmark = styled.div`
  --leo-icon-size: 17px;
`

export const text = styled.div`
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: ${leo.color.text.secondary};
`
