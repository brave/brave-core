/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

export const root = styled.div`
  display: flex;
  align-items: center;
  gap: 16px;
`

export const batIcon = styled.div`
  width: 40px;
  height: 40px;
  background: #F2F4F7; /* Same in light/dark */
  border-radius: 50%;
  padding: 6px;

  .icon {
    margin-top: -1px;
  }
`

export const title = styled.div`
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
  display: flex;
  gap: 8px;
`

export const provider = styled.div`
  background: ${leo.color.gray['10']};
  border-radius: 4px;
  padding: 3px 6px;

  font-weight: 500;
  font-size: 11px;
  line-height: 18px;
  letter-spacing: 0.02em;
  text-transform: uppercase;
  color: ${leo.color.gray['50']};

  .icon {
    height: 12px;
    width: auto;
    vertical-align: middle;
    margin-top: -2px;
    padding-right: 4px;
  }
`

export const amount = styled.div`
  font-weight: 600;
  font-size: 16px;
  line-height: 28px;
  margin-top: -4px;
  color: ${leo.color.text.primary};
`
