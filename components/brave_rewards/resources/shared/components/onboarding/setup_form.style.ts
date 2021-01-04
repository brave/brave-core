/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const panelText = styled.div`
  color: var(--brave-color-brandBat);
  font-size: 14px;
  line-height: 20px;
  margin-top: -5px;
`

export const root = styled.div`
`

export const section = styled.div`
  margin-top: 30px;
  font-size: 14px;
  line-height: 21px;

  &:first-of-type {
    margin-top: 0px;
  }
`

export const label = styled.div`
  color: var(--brave-palette-neutral900);
`

export const sublabel = styled.div`
  color: var(--brave-palette-neutral600);
`

export const optionBar = styled.div`
  margin-top: 10px;
  background: rgba(241, 236, 255, 0.5);
  border: 2px solid var(--brave-palette-purple300);
  border-radius: 2px;
  display: flex;

  button {
    margin: 3px;
    flex: 1 1 auto;
    font-weight: 600;
    font-size: 18px;
    line-height: 27px;
    color: var(--brave-palette-neutral700);
    border: 0;
    border-radius: 2px;
    background: none;
    cursor: pointer;

    &.selected {
      color: var(--brave-palette-white);
      background: var(--brave-palette-purple400);
      cursor: default;

      * {
        color: var(--brave-palette-white);
      }
    }
  }

  button.large-text {
    font-size: 22px;
    line-height: 39px;
  }
`

export const acAmount = styled.span`
  font-weight: 600;
  font-size: 18px;
  line-height: 40px;
`

export const acCurrency = styled.span`
  font-weight: normal;
  font-size: 12px;
  line-height: 18px;
`
