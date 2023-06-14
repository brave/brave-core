/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

const shadowBox = `
  padding: 16px;
  background: #FFFFFF;
  box-shadow: 0px 0px 1px rgba(0, 0, 0, 0.08),
              0px 0.5px 1.5px rgba(0, 0, 0, 0.1);
  border-radius: 8px;
  color: #495057;

  .brave-theme-dark & {
    background: #1E2029;
    color: #C2C4CF;
  }
`

export const root = styled.div`
  // We want the padding to be 16px but the WebUI bubble already includes
  // a padding of 13px, so just specify the additional 3px here
  padding: 3px;

  a {
    text-decoration: none;
  }
`

export const header = styled.div`
  display: flex;
  align-items: center;
`

export const title = styled.div`
  flex: 1 1 auto;
  font-weight: 600;
  font-size: 20px;
  line-height: 30px;
  color: #212529;

  .brave-theme-dark & {
    color: #F0F2FF;
  }
`

export const headerText = styled.div`
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: #868E96;
  .brave-theme-dark & {
    color: #84889C;
  }
`

export const connect = styled.div`
  margin-top: 16px;
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 16px;
  background: linear-gradient(129.97deg, #4C54D2 9.56%, #A3278F 106.62%);
  border-radius: 8px;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  color: #fff;

  strong {
    font-weight: 600;
  }
`

export const connectAction = styled.div`
  button {
    ${mixins.buttonReset}
    background: rgba(255, 255, 255, 0.24);
    border-radius: 48px;
    padding: 6px 14px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    color: #fff;
    cursor: pointer;

    &:hover {
      background: rgba(255, 255, 255, 0.30);
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

export const connectLearnMore = styled.div`
  margin-top: -2px;

  a {
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    color: #fff;
  }
`

export const publisherSupport = styled.div`
  ${shadowBox}
  display: flex;
  gap: 16px;
  align-items: center;
  margin-top: 16px;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
`

export const publisherCount = styled.div`
  font-weight: 400;
  font-size: 40px;
  line-height: 40px;
`

export const settings = styled.div`
  margin-top: 16px;

  button {
    ${mixins.buttonReset}
    width: 100%;
    padding: 10px;
    border: solid 1px #AEB1C2;
    border-radius: 48px;
    background: transparent;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    color: #212529;
    cursor: pointer;

    &:hover {
      background: rgba(0, 0, 0, 0.02);
    }

    .brave-theme-dark & {
      border-color: #5E6175;
      color: #F0F2FF;

      &:hover {
        background: rgba(255, 255, 255, 0.05);
      }
    }

    .icon {
      height: 18px;
      width: 18px;
      vertical-align: middle;
      margin-top: -3px;
      margin-right: 8px;
    }
  }
`

export const learnMore = styled.div`
  margin-top: 18px;
  text-align: center;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;

  a {
    color: #4C54D2;
  }
`
