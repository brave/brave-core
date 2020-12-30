/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import backgroundBG from './assets/promo_bg.svg'

export const root = styled.div`
  width: 100%;
  position: relative;
  background-image:
    url('${backgroundBG}'),
    linear-gradient(279.66deg, #694CD9 0%, #7D186C 100%);
  background-repeat: no-repeat;
  background-size: cover;
  border-radius: 12px;
  font-family: var(--brave-font-heading);
  color: var(--brave-palette-white);
  padding: 28px 16px 18px;
`

export const close = styled.div`
  button {
    position: absolute;
    top: 10px;
    right: 10px;
    background: none;
    border: none;
    padding: 0;
    margin: 0;
    cursor: pointer;

    .icon {
      color: var(--brave-palette-white);
      height: 12px;
      width: 12px;
    }
  }
`

export const header = styled.div`
  font-weight: 600;
  font-size: 21px;
  line-height: 18px;
`

export const text = styled.div`
  margin-top: 6px;
  font-size: 14px;
  line-height: 20px;
  color: var(--brave-palette-neutral000);
`

export const action = styled.div`
  margin-top: 12px;

  button {
    background: var(--brave-palette-white);
    border-radius: 32px;
    font-weight: 600;
    font-size: 13px;
    line-height: 22px;
    padding: 5px 20px;
    color: var(--brave-color-brandBat);
    border: 0;
    cursor: pointer;
  }
`
