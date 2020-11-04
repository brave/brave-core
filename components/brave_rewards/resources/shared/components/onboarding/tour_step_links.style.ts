/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  text-align: center;
  display: flex;
  justify-content: center;

  button {
    margin: 0 2px;
    padding: 0 3px;
    height: 8px;
    background: none;
    border: none;
    cursor: pointer;
    outline-offset: 2px;
    outline-style: none;

    &:focus-visible {
      outline-style: auto;
    }

    &::before {
      content: ' ';
      display: block;
      height: 8px;
      width: 8px;
      border-radius: 50%;
      background: var(--brave-palette-grey200);
    }

    &.selected::before {
      background: var(--brave-color-brandBat);
    }
  }
`
