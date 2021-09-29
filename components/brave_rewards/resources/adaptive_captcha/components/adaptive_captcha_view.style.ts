/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const frameBox = styled.div`
  iframe {
    width: 323px;
    height: 275px;
    font-family: var(--brave-font-heading);
    background: var(--brave-palette-white);
    border: 0;
    margin-right: 4px;
  }
`

export const root = styled.div`
  width: 332px;
  padding: 30px 16px 30px 25px;
  font-family: var(--brave-font-heading);
  background: var(--brave-palette-white);
  color: var(--brave-palette-neutral900);
`

export const title = styled.div`
  margin-top: 16px;
  font-weight: 600;
  font-size: 22px;
  line-height: 33px;

  img {
    width: 24px;
    height: 24px;
    margin-right: 8px;
    vertical-align: middle;
    margin-bottom: 3px;
  }

  &.long {
    font-size: 18px;
    line-height: 27px;
  }
`

export const text = styled.div`
  margin-top: 5px;
  font-weight: 400;
  font-size: 16px;
  line-height: 24px;
`

export const closeAction = styled.div`
  margin-top: 26px;
  margin-bottom: 5px;

  button {
    display: block;
    margin: 0 auto;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    color: inherit;
    border: 0;
    padding: 0;
    background: none;
    cursor: pointer;

    &:hover {
      text-decoration: underline;
    }
  }
`

export const helpAction = styled.div`
  margin-top: 36px;

  button {
    display: block;
    height: 40px;
    margin: 0 auto;
    padding: 10px 22px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    background: var(--brave-palette-white);
    color: #212529;
    border: 1px solid var(--brave-palette-grey500);
    box-sizing: border-box;
    border-radius: 48px;
    cursor: pointer;

    &:active {
      background: var(--brave-palette-grey000);
    }
  }
`
