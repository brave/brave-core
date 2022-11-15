/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  button {
    color: var(--brave-palette-white);
    background: var(--brave-color-brandBat);
    border: none;
    padding: 10px 40px;
    border-radius: 21px;
    font-weight: 600;
    font-size: 14px;
    line-height: 21px;
    cursor: pointer;

    &:active {
      background: var(--brave-color-brandBatActive);
    }

    &[disabled] {
      background: var(--brave-palette-grey200);
      cursor: default;
    }
  }
`
