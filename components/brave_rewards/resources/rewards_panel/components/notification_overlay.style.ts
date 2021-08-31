/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  position: fixed;
  display: flex;
  justify-content: center;
  align-items: flex-end;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  background: rgba(0, 0, 0, 0.15);
  overflow: hidden;
`

export const card = styled.div`
  flex: 0 1 376px;
  position: relative;
  margin: 0 10px 20px;
  z-index: 0;

  animation-name: slide-in;
  animation-easing-function: ease-out;
  animation-duration: 250ms;
  animation-fill-mode: both;

  @keyframes slide-in {
    from { transform: translate(0, calc(100% + 20px)); }
    to { transform: translate(0, 0); }
  }
`

export const peek = styled.div`
  position: absolute;
  background: var(--brave-palette-neutral000);
  opacity: .72;
  border-radius: 8px;
  height: 20px;
  bottom: -5px;
  right: 34px;
  left: 34px;
  z-index: -1;

  .brave-theme-dark & {
    background: var(--brave-palette-grey700);
  }
`
