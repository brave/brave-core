/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  button {
    position: relative;
    height: var(--toggle-button-height, 24px);
    width: var(--toggle-button-width, 48px);
    background: #DADCE8;
    border: none;
    border-radius: 32px;
    margin: 0;
    padding: 0;
    vertical-align: middle;
    cursor: pointer;

    --toggle-handle-margin: 3px;
    --toggle-handle-size: calc(var(--toggle-button-height, 24px) - 6px);

    .brave-theme-dark & {
      background: #84889C;
    }

    &.checked {
      background: #4C54D2;
    }
  }
`

export const handle = styled.div`
  position: absolute;
  top: var(--toggle-handle-margin);
  left: var(--toggle-handle-margin);
  background: #fff;
  height: var(--toggle-handle-size);
  width: var(--toggle-handle-size);
  border-radius: 50%;
  transition: all .3s ease;

  button.checked & {
    right: 0;
    left: calc(100% - var(--toggle-handle-size) - var(--toggle-handle-margin));
  }
`
