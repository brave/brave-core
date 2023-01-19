/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../lib/css_mixins'

export const formText = styled.div`
  color: var(--brave-color-brandBat);
  font-size: 14px;
  line-height: 20px;
  margin-top: -5px;
`

export const verifySubtext = styled.div`
  font-size: 12px;
  line-height: 16px;
  color: var(--brave-palette-neutral700);
`

export const verifyActions = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  gap: 16px;

  button {
    ${mixins.buttonReset}
    background: #4C54D2;
    border-radius: 48px;
    padding: 6px 18px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    color: #fff;
    cursor: pointer;

    &:active {
      background: #737ADE;
    }

    .icon {
      vertical-align: middle;
      height: 17px;
      width: auto;
      margin-left: 8px;
      margin-top: -2px;
    }
  }

  button.verify-later {
    ${mixins.buttonReset}
    color: #4C54D2;
    font-weight: 600;
    font-size: 12px;
    line-height: 20px;
    cursor: pointer;
  }
`
