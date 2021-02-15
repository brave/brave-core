/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import bannerBackground from './assets/alert_background.svg'

export const root = styled.div`
  background-image: url('${bannerBackground}');
  background-repeat: no-repeat;
  background-size: contain;
  background-color: var(--brave-palette-white);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.2);
  border-radius: 8px;
  width: 440px;
  padding: 12px;
  font-family: var(--brave-font-heading);

  .icon {
    color: #fff;
  }
`

export const banner = styled.div`
  color: var(--brave-palette-white);
  text-align: center;
  margin-top: -12px;

  .icon {
    height: 35px;
    width: auto;
  }
`

export const content = styled.div`
  padding: 30px 22px 18px;
`

export const header = styled.div`
  text-align: center;
  font-weight: 600;
  font-size: 22px;
  line-height: 32px;
`

export const text = styled.div`
  margin-top: 9px;
  font-size: 14px;
  line-height: 22px;
  color: var(--brave-palette-neutral700);
`

export const action = styled.div`
  margin-top: 25px;
  text-align: center;

  button {
    color: var(--brave-palette-white);
    min-width: 160px;
    padding: 10px 20px;
    font-weight: 600;
    font-size: 14px;
    line-height: 22px;
    border: none;
    border-radius: 40px;
    background: var(--brave-color-brandBat);
    cursor: pointer;
    text-transform: uppercase;
  }

  button:active {
    background: var(--brave-color-brandBatActive);
  }
`
