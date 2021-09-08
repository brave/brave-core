/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { buttonReset } from '../../shared/lib/css_mixins'

export const root = styled.div`
  margin: 10px 20px 0px;

  button {
    ${ buttonReset }
    width: 100%;
    font-family: var(--brave-font-heading);
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    color: var(--brave-palette-neutral900);
    border: 1px solid var(--brave-palette-grey500);
    border-radius: 48px;
    padding: 6px 18px;
    cursor: pointer;

    &:active {
      background: rgba(0, 0, 0, .01);
    }
  }

  .icon {
    width: 17px;
    height: auto;
    vertical-align: middle;
    margin-right: 8px;
    margin-bottom: 3px;
  }
`
