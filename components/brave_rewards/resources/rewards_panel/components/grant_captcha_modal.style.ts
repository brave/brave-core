/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  width: 377px;
  background:
    linear-gradient(180deg, rgba(255, 255, 255, 0.5) 61.69%, #E9E9F4 102.5%),
    var(--brave-palette-white);
  padding: 17px;
  border-radius: 16px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.2);
  text-align: center;
  font-size: 14px;
  line-height: 18px;
`

export const header = styled.div`
  margin-top: 8px;
  margin-bottom: 9px;
  font-weight: 600;
  font-size: 18px;
  line-height: 27px;
  color: var(--brave-palette-neutral900);
`

export const text = styled.div``

export const summary = styled.div`
  margin: 20px 30px 0;
  border-radius: 16px;
  background: rgba(99, 105, 110, 0.04);
  border: solid 1px var(--brave-palette-neutral200);
  padding: 25px 10px;
`

export const summaryItem = styled.div`
  color: var(--brave-palette-neutral700);
  margin-bottom: 28px;
  font-size: 16px;
  line-height: 20px;

  &:last-child {
    margin-bottom: 0;
  }
`

export const summaryValue = styled.div`
  margin-top: 5px;
  font-weight: 600;
  color: var(--brave-palette-neutral900);
`

export const okButton = styled.div`
  margin: 25px 30px 15px;

  button {
    width: 100%;
    background: var(--brave-color-brandBatInteracting);
    color: var(--brave-palette-white);
    padding: 10px;
    margin: 0;
    border: none;
    border-radius: 48px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    cursor: pointer;

    &:active {
      background: var(--brave-palette-blurple400);
    }
  }
`
