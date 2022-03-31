/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  button {
    position: relative;
    height: 24px;
    width: 48px;
    background: #E1E2F6;
    border: none;
    border-radius: 32px;
    margin: 0;
    padding: 0;
    vertical-align: middle;
    cursor: pointer;

    .brave-theme-dark & {
      background: #7679B1;
    }
  }
`

export const handle = styled.div`
  position: absolute;
  top: 2px;
  left: 2px;
  background-color: rgba(0, 0, 0, .3);
  height: 20px;
  width: 20px;
  border-radius: 50%;
  transition: all .3s ease;

  &.checked {
    right: 0;
    left: calc(100% - 22px);
    background-color: var(--brave-color-brandBatInteracting);

    .brave-theme-dark & {
      background-color: #4436E1;
    }
  }
`
