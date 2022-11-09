/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../lib/css_mixins'

export const root = styled.div`
  padding: 20px 16px;
  font-family: var(--brave-font-heading);
  text-align: center;
  background: linear-gradient(137.04deg, #346FE1 33.4%, #5844C3 82.8%);
  border-radius: 8px;
  color: var(--brave-palette-white);;
`

export const header = styled.div`
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;

  .icon {
    vertical-align: middle;
    width: 16px;
    margin-right: 5px;
    margin-bottom: 3px;
  }
`

export const text = styled.div`
  margin-top: 9px;
  font-size: 12px;
  line-height: 18px;

  a {
    color: var(--brave-palette-white);
    text-decoration: underline;
    padding-left: 4px;
  }
`

export const enable = styled.div`
  margin-top: 16px;

  button {
    ${mixins.buttonReset}
    color: var(--brave-palette-white);
    background: rgba(255, 255, 255, 0.2);
    border-radius: 48px;
    width: 100%;
    padding: 6px;
    font-size: 13px;
    line-height: 20px;
    font-weight: 600;
    font-size: 12px;
    line-height: 20px;
    cursor: pointer;

    &:active {
      background: rgba(255, 255, 255, 0.3);
    }
  }
`

export const terms = styled.div`
  margin-top: 16px;
  font-size: 11px;
  line-height: 16px;
  color: rgba(255, 255, 255, 0.75);

  a {
    color: var(--brave-color-white);
    text-decoration: underline;
  }
`
