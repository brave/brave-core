/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.span`
  font-weight: 600;
  white-space: nowrap;

  a {
    text-decoration: none;
    color: #3b3e4f;
  }
`

export const icon = styled.span`
  position: relative;
  margin-right: 12px;

  img {
    display: inline-block;
    vertical-align: middle;
    height: 24px;
    width: auto;
    margin-top: -2px;
  }
`

export const verified = styled.div`
  position: absolute;
  top: -6px;
  right: -8px;

  .icon {
    width: 16px;
    height: auto;
    color: #4C54D2;
    background-color: #FFFFFF;
    border-radius: 50%;
  }
`

export const platform = styled.span``
