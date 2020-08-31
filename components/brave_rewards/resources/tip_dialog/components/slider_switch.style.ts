/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  position: relative;
`

export const bar = styled.div`
  z-index: 0;
  position: absolute;
  margin: 4px;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;

  background: #fff;
  border-radius: 21px;
  transition: all .3s ease;
`

export const rail = styled.div`
  display: flex;
  align-items: center;
  height: 37px;

  background: #DFDFE8;
  border-radius: 21px;
`

export const option = styled.div`
  z-index: 1;
  flex: 1 1 0;

  button {
    width: 100%;
    text-align: center;

    font-size: 12px;
    line-height: 30px;
    color: #686978;
    background: transparent;
    cursor: pointer;
    border: none;
  }

  &.selected button {
    position: relative;

    color: var(--slider-switch-selected-color, #4C54D2);
    font-weight: 600;
    cursor: default;
  }
`
