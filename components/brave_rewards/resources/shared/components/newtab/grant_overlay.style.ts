/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { buttonReset } from '../../lib/css_mixins'

export const root = styled.div`
  margin: 16px -4px 0;
  background: rgba(116, 210, 215, 0.1);
  border-radius: 8px;
  padding: 20px 25px 24px;
`

export const title = styled.div`
  margin-top: 16px;
  font-weight: 600;
  font-size: 14px;
  line-height: 18px;
  text-align: center;
  color: var(--brave-palette-white);
`

export const amount = styled.div`
  margin-top: 8px;
  font-size: 14px;
  line-height: 20px;
  text-align: center;
  color: var(--brave-palette-grey400);

  .amount, .currency {
    font-weight: 600;
  }
`

export const action = styled.div`
  margin: 16px 0;

  button {
    width: 100%;

    ${buttonReset}
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    cursor: pointer;
    background: var(--brave-palette-blurple500);
    border-radius: 48px;
    padding: 10px;
    text-align: center;

    &:hover {
      background:
        linear-gradient(rgba(15, 28, 45, .05), rgba(15, 28, 45, .1)),
        var(--brave-palette-blurple500);
    }

    &:active {
      background:
        linear-gradient(rgba(15, 28, 45, .08), rgba(15, 28, 45, .12)),
        var(--brave-palette-blurple500);
    }
  }
`

export const text = styled.div`
  font-size: 14px;
  line-height: 20px;
  text-align: center;
  color: var(--brave-palette-grey400);
`

export const days = styled.span`
  font-weight: 600;
  color: var(--brave-palette-white);
`
