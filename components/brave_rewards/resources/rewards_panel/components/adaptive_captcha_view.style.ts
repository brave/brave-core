/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const overlay = styled.div`
  position: fixed;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  overflow: auto;
  z-index: 9999;
  display: flex;
  flex-direction: column;
  justify-content: space-around;
  align-items: center;
  background: rgba(0, 0, 0, 0.33);
`

export const frameRoot = styled.div`
  iframe {
    width: 332px;
    height: 275px;
    font-family: var(--brave-font-heading);
    background: var(--brave-palette-white);
    border-radius: 16px;
    border: 0;
  }
`

export const modalRoot = styled.div`
  width: 332px;
  padding: 30px 16px 30px 25px;
  font-family: var(--brave-font-heading);
  background: var(--brave-palette-white);
  color: var(--brave-palette-neutral900);
  border-radius: 16px;
`

export const title = styled.div`
  display: flex;
  margin-top: 16px;
  font-weight: 600;
  font-size: 22px;
  line-height: 33px;

  img {
    width: 24px;
    height: 24px;
    margin-right: 8px;
    vertical-align: middle;
    margin-top: auto;
    margin-bottom: auto;
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
  text-align: center;

  button {
    display: inline-block;
    padding: 10px 22px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    background: var(--brave-palette-white);
    color: #212529;
    border: 1px solid var(--brave-palette-grey500);
    border-radius: 48px;
    cursor: pointer;
    text-decoration: none;

    &:active {
      background: var(--brave-palette-grey000);
    }
  }
`
