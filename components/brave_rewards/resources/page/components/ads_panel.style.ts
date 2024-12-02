/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div``

export const description = styled.div`
  color: var(--brave-palette-neutral600);
  margin: 16px 0;
  line-height: 24px;

  a {
    padding-left: 4px;
    text-decoration: none;
  }
`

export const terms = styled.div`
  color: #3b3e4f;
  margin: 20px calc(-1 * var(--settings-panel-padding)) 0;
  padding: 0 15px 15px 32px;
  border-bottom: 1px solid rgba(184, 185, 196, 0.4);
  font-size: 13px;
`

export const paymentStatus = styled.div`
  margin-bottom: 8px;
  color: var(--brave-palette-neutral900);
  font-size: 14px;
  line-height: 24px;

  a {
    color: var(--brave-palette-blurple500);
    font-weight: 500;
    text-decoration: none;
  }

  > div {
    background: #E8F4FF;
    border-radius: 4px;
    padding: 6px 21px;
    display: flex;
    gap: 6px;
    font-weight: 500;
  }

  .rewards-payment-pending {
    background: #E8F4FF;

    .icon {
      color: var(--brave-palette-blue500);
      height: 16px;
      width: auto;
      vertical-align: middle;
      margin-bottom: 1px;
    }
  }

  .rewards-payment-completed {
    background: #E7FDEA;
    align-items: center;

    .icon {
      height: 20px;
      width: auto;
      vertical-align: middle;
    }
  }
`

export const earningsRow = styled.div`
  display: none;
`

export const earnings = styled.span`
  color: #495057;
`

export const hiddenEarnings = styled.div`
  white-space: nowrap;

  a {
    display: inline-block;
    vertical-align: middle;
    margin-left: 8px;
    margin-top: -3px;
    color: #4C54D2;
    text-decoration: none;
    font-weight: 600;
    font-size: 13px;
    line-height: 18px;
    text-decoration: none;
  }
`

export const totalAdsCount = styled.span`
  font-weight: 600;
`

export const showHistory = styled.div`
  margin-top: 25px;
  text-align: right;

  button {
    ${mixins.buttonReset}
    cursor: pointer;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: -0.01em;
    color: #4C54D2;
  }

  .layout-narrow & {
    display: none;
  }
`

export const needsUpdate = styled.div`
  display: flex;
  align-items: center;
  justify-content: start;
  background: #FDF1F2;
  padding: 5px;
  border-radius: 6px;
  margin-bottom: 25px;
`

export const needsUpdateIcon = styled.div`
  .icon {
    width: 30px;
    height: 30px;
    margin: 8px;
    color: var(--brave-palette-red500);
  }
`

export const needsUpdateContent = styled.div`
  display: block;
`

export const needsUpdateHeader = styled.div`
  margin: 5px;
  font-size: 16px;
  font-weight: 600;
  color: var(--brave-palette-neutral800);
`

export const needsUpdateBody = styled.div`
  margin: 5px;
  font-size: 15px;
  color: var(--brave-palette-neutral800);
`

export const notSupported = styled.div`
  margin-top: 25px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #f0f8ff;
  padding: 12px;
  border-radius: 6px;
  font-size: 16px;
  line-height: 20px;
  color: #5e6175;
`

export const notSupportedIcon = styled.div`
  .icon {
    display: block;
    width: 24px;
    height: 24px;
    margin: 8px;
    color: #339af0;
  }
`

export const connect = styled.div`
  margin-bottom: 8px;
  background: rgba(93, 181, 252, 0.2);
  border-radius: 8px;
  padding: 16px;
  font-size: 14px;
  line-height: 20px;

  strong {
    font-weight: 600;
  }
`

export const connectAction = styled.div`
  margin-top: 16px;

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
