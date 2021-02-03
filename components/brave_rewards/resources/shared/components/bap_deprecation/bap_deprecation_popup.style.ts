/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  background: linear-gradient(328.73deg, #A1A8F2 8.09%, #4C54D2 97.6%);
  color: var(--brave-palette-white);
  font-family: var(--brave-font-heading);
  font-size: 14px;
  font-weight: 600;
  line-height: 22px;
  width: 248px;
  padding: 28px 16px 16px;

  .icon {
    color: var(--brave-palette-white);
  }
`

export const content = styled.div``

export const heading = styled.div`
  .icon {
    vertical-align: middle;
    margin-bottom: 3px;
    margin-right: 4px;
    height: 20px;
    width: 20px;
  }
`

export const text = styled.div`
  color: rgba(255, 255, 255, 0.7);
`

export const action = styled.div`
  text-align: center;
  margin-top: 16px;

  button {
    cursor: pointer;
    font-weight: 600;
    font-size: 13px;
    line-height: 19px;
    padding: 10px 40px;
    border: solid 1px var(--brave-palette-white);
    border-radius: 26px;
    background: none;
  }

  button:active {
    background: #A1A8F2;
  }
`
