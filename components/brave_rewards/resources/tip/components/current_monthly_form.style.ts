/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  height: 100%;
  padding-top: 45px;
  display: flex;
  flex-direction: column;
  color: var(--brave-palette-neutral900);
`

export const header = styled.div`
  text-align: center;
  font-weight: 600;
  font-size: 16px;
  line-height: 24px;
`

export const contributionTable = styled.div`
  font-size: 14px;
  line-height: 21px;

  table {
    width: calc(100% - 84px);
    margin: 0 auto;
    max-width: 360px;
  }

  td {
    padding: 16px 0 0 0;
  }

  td:last-child {
    text-align: right;
  }
`

export const buttons = styled.div`
  margin-top: 62px;
  text-align: center;
`

export const cancel = styled.span`
  margin-right: 9px;

  button {
    padding: 10px 22px;
    border: none;
    background: none;
    color: var(--brave-palette-neutral600);
    cursor: pointer;
    font-weight: 600;
    font-size: 14px;
    line-height: 21px;
  }
`

export const changeAmount = styled.span`
  button {
    padding: 9px 21px;
    background: none;
    color: var(--brave-color-brandBatInteracting);
    cursor: pointer;
    font-weight: 600;
    font-size: 14px;
    line-height: 21px;
    border: 1px solid rgba(154, 160, 255, 0.55);
    border-radius: 30px;
  }

  button:active {
    background: var(--brave-color-brandBatActive);
  }
`

export const cancelText = styled.div`
  margin: 24px 54px 0;
  text-align: center;
  flex: 1 1 auto;
`
