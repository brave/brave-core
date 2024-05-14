/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

export const root = styled.div`
  text-align: center;
  font-weight: 400;
  font-size: 11px;
  line-height: 16px;
  color: ${leo.color.text.secondary};

  a {
    color: ${leo.color.text.primary};
    text-decoration: none;
  }
`
