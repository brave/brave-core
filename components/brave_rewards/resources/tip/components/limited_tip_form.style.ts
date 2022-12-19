/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  display: flex;
  flex-direction: column;
  height: 100%;
`

export const loading = styled.div``

export const header = styled.div`
  text-align: center;
  padding-top: 64px;
  padding-left: 32px;
  padding-right: 32px;
  font-size: 18px;
  font-weight: 600;
  line-height: 26px;
`

export const headerSubtitle = styled.div`
  padding-top: 16px;
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
`

export const tipCreatorsGraphic = styled.div`
  margin-top: 44px;
  margin-left: 80px;
  margin-right: 65px;
  height: 120px;
  width: 217px;
`

export const connectAction = styled.div`
  margin-top: 44px;
  margin-left: 32px;
  margin-right: 32px;

  button {
    ${mixins.buttonReset}
    background: #4C54D2;
    border-radius: 48px;
    width: 299px;
    height: 40px;
    padding: 10px 22px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    color: #fff;
    cursor: pointer;

    .icon {
      vertical-align: middle;
      height: 15px;
      width: auto;
      margin-left: 8px;
      margin-top: -2px;
    }
  }
`
