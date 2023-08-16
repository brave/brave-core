/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css'

export const alert = styled.div`
  margin-top: 16px;
  padding: 16px;
  border-radius: 8px;
  background: ${leo.color.systemfeedback.errorBackground};
  font-size: 14px;
  font-weight: 400;
  line-height: 24px;
  display: flex;
  gap: 16px;

  a {
    font-weight: 600;
    text-decoration: none;
    color: ${leo.color.icon.interactive};
  }
`

export const alertIcon = styled.div`
  padding-top: 2px;
  color: ${leo.color.systemfeedback.errorIcon};
  --leo-icon-size: 20px;
`
