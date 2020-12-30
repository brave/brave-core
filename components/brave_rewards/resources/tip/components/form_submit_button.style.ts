/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  button {
    width: 100%;
    padding: 19px 0;
    cursor: pointer;
    border: none;
    background: var(--brave-color-brandBatInteracting);
    font-size: 14px;
    line-height: 21px;
    font-weight: 600;
    color: var(--brave-palette-white);
    outline: none;
  }

  button:active {
    background: var(--brave-palette-blurple400);
  }

  button:focus-visible {
    outline: solid 2px rgba(255, 255, 255, .7);
    outline-offset: -2px;
  }

  .icon {
    height: 16px;
    vertical-align: middle;
    margin-right: 5px;
    margin-top: var(--icon-margin-top, 0);
  }
`
