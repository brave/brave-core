/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  --settings-panel-title-color: #696fdc;
`

export const description = styled.div`
  color: var(--brave-palette-neutral600);
  margin: 16px 0;
  line-height: 24px;
`

export const showAll = styled.div`
  margin-top: 25px;
  text-align: right;

  button {
    ${mixins.buttonReset}
    cursor: pointer;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: -0.01em;
    color: #4C54D2;
  }
`

export const restart = styled.div`
  text-align: center;
  margin-top: 32px;

  button {
    ${mixins.buttonReset}
    padding: 7px 20px;
    font-weight: 500;
    font-size: 12px;
    color: #212529;
    background: ;
    border: 1px solid #AEB1C2;
    border-radius: 20px;
    cursor: pointer;

    &:hover {
      background: rgba(0, 0, 0, 0.02);
    }
  }
`
