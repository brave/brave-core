/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  text-align: center;
`

export const text = styled.div`
  font-size: 14px;
  line-height: 21px;
  font-weight: 500;
  color: var(--brave-palette-neutral900);
`

export const amount = styled.div`
  position: relative;
  margin-top: 32px;
  min-width: 178px;
  display: inline-block;
  background: var(--brave-color-brandBatInteracting);
  padding: 10px 60px;
  color: var(--brave-palette-white);
  font-weight: 600;
  font-size: 14px;
  line-height: 21px;
  border-radius: 30px;
`

export const currency = styled.span`
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
`

export const reset = styled.div`
  button {
    border: 0;
    background: rgba(174, 177, 194, .24);
    width: 32px;
    height: 32px;
    position: absolute;
    top: 4px;
    right: 4px;
    cursor: pointer;
    border-radius: 50%;

    .icon {
      color: var(--brave-palette-white);
      opacity: .8;
      transform: rotate(45deg);
      font-size: 28px;
      line-height: 32px;
    }
  }
`

export const exchange = styled.div`
  margin-top: 6px;
  color: var(--brave-color-brandBatInteracting);
  font-size: 12px;
  line-height: 18px;
`
