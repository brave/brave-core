/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

export const root = styled.div`
  max-width: 550px;
  padding: 0 16px 8px;
  display: flex;
  flex-direction: column;
  gap: 24px;

  a {
    font-weight: 600;
    text-decoration: none;
  }
`

export const title = styled.div`
  align-self: center;
  font: ${leo.font.heading.h3};
`

export const text = styled.div`
  font: ${leo.font.default.regular};
`

export const consent = styled.div``

export const consentLabel = styled.div`
  font: ${leo.font.default.regular};
`

export const action = styled.div`
  margin-top: 8px;
  align-self: center;
`
