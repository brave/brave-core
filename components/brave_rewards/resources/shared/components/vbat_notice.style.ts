/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../lib/css_mixins'

export const root = styled.div``

export const header = styled.div`
  background: #FFD43B;
  padding: 24px;
  gap: 18px;
  display: flex;
  flex-wrap: wrap;

  color: #212529;
  font-weight: 600;
  font-size: 20px;
  line-height: 30px;
`

export const headerIcon = styled.div`
  .icon {
    width: 26px;
    height: auto;
    vertical-align: middle;
  }
`

export const headerClose = styled.div`
  flex: 1 0 51%;
  text-align: right;

  .icon {
    width: 13px;
    height: auto;
    vertical-align: middle;
    margin-top: -1px;
  }

  button {
    ${mixins.buttonReset}
    cursor: pointer;
  }
`

export const headerText = styled.div`
  flex: 1 0 51%;
`

export const content = styled.div`
  padding: 24px;
  background: #fff;
  color: #212529;
  font-size: 14px;
  line-height: 20px;
`

export const actions = styled.div`
  margin-top: 16px;
  display: flex;
  align-items: center;
  gap: 16px;
`

export const connectAction = styled.div`
  button {
    ${mixins.buttonReset}
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    background: #4C54D2;
    border-radius: 48px;
    padding: 10px 22px;
    color: #fff;
    cursor: pointer;

    &:active {
      background: #737ADE;
    }

    .icon {
      vertical-align: middle;
      height: 17px;
      width: auto;
      margin-left: 8px;
      margin-top: -2px;
    }
  }
`

export const learnMoreAction = styled.div`
  flex: 1 1 auto;

  a {
    color: #4C54D2;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    text-decoration: none;
  }
`
