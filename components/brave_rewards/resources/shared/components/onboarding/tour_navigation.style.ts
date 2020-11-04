/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: center;

  button {
    font-weight: 600;
    font-size: 13px;
    line-height: 19px;
    cursor: pointer;
    border: none;
    background: none;
    outline-style: none;
  }

  button:focus-visible {
    outline-style: auto;
  }

  button.nav-forward {
    color: var(--brave-palette-white);
    background: var(--brave-color-brandBat);
    padding: 5px 20px;
    border-radius: 30px;

    .icon {
      height: 12px;
      vertical-align: middle;
      margin: 0 -6px 2px 2px;
    }

    &:active {
      background: var(--brave-color-brandBatActive);
    }
  }

  button.nav-skip {
    color: var(--brave-palette-neutral600);
  }

  button.nav-back {
    padding: 5px;
    color: var(--brave-color-brandBat);

    .icon {
      height: 12px;
      vertical-align: middle;
      margin: 0 2px 2.5px 0;
    }
  }

  button.nav-start {
    padding: 10px 26px;
  }
`

export const narrowStart = styled.div`
  width: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;

  button.nav-skip {
    margin-top: 16px;
  }
`
