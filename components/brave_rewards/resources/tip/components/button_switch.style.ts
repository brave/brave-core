/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
`

export const option = styled.div`
  flex: 1 1 auto;
  padding: 0 5px;

  button {
    width: 100%;
    padding: var(--button-switch-padding, 6px 0);
    font-size: 12px;
    line-height: 21px;
    border-radius: 30px;
    border: 1px solid rgba(115, 122, 222, 0.55);
    background: var(--brave-palette-white);
    color: var(--brave-color-brandBatInteracting);
    cursor: pointer;
    outline: none;
  }

  button:active {
    background: var(--brave-color-brandBatActive);
    color: var(--brave-color-brandBatInteracting);
  }

  &.selected button {
    background: var(--brave-color-brandBatInteracting);
    color: var(--brave-palette-white);
    cursor: default;
  }

  button:focus-visible {
    outline-style: auto;
    outline-color: var(--brave-color-brandBatInteracting);
    outline-offset: 2px;
  }
`

export const caption = styled.div`
  margin-top: 5px;
  text-align: center;
  font-size: 12px;
  line-height: 18px;
  color: var(--brave-palette-grey700);
`
