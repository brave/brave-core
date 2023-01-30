/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  background: linear-gradient(125.83deg, #392DD1 0%, #22B8CF 99.09%);
  border-radius: 8px;
  overflow: hidden;
  color: #fff;
  text-align: center;
`

export const close = styled.div`
  text-align: right;
  padding: 6px 6px 0 0;

  button {
    ${mixins.buttonReset}
    padding: 5px;
    cursor: pointer;
  }

  .icon {
    display: block;
    height: 12px;
    width: auto;
  }
`

export const content = styled.div`
  padding: 6px 26px 12px 26px;
`

export const title = styled.div`
  font-size: 20px;
  font-weight: 600;
  line-height: 26px;
  letter-spacing: 0.15px;
  margin-bottom: 9px;
`

export const text = styled.div`
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  margin-bottom: 24px;
`

export const learnMore = styled.div`
  text-align: center;

  button {
    ${mixins.buttonReset}
    display: block;
    margin: 10px auto;
    padding: 6px 18px;
    max-width: 320px;
    width: 100%;
    border-radius: 48px;
    font-weight: 600;
    font-size: 12px;
    line-height: 18px;
    letter-spacing: 0.01em;
    color: #4C54D2;
    background: #fff;
    cursor: pointer;
  }
`

export const dismiss = styled.div`
  text-align: center;

  button {
    ${mixins.buttonReset}
    font-size: 12px;
    line-height: 20px;
    letter-spacing: 0.01em;
    color: #fff;
    cursor: pointer;
  }
`

export const image = styled.div`
  margin: 8px;
  border-radius: 8px;
  overflow: hidden;

  img {
    width: calc(100% + 4px);
    height: auto;
    margin-bottom: -5px;
    margin-left: -2px;
    margin-top: -2px;
  }
`
