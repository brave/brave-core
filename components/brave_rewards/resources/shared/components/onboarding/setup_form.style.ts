/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../lib/css_mixins'

import graphicImage from './assets/tour_setup.svg'

export const root = styled.div`
  margin-top: -2px;
`

export const label = styled.div`
  font-weight: 500;
  font-size: 15px;
  line-height: 20px;
  color: var(--brave-palette-neutral900);
`

export const sublabel = styled.div`
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  margin: 4px 0;
  color: var(--brave-palette-neutral600);
`

export const graphic = styled.div`
  margin-top: 22px;
  height: 100px;
  background: center no-repeat url(/${graphicImage});
  background-size: contain;

  .tour-wide & {
    height: 82px;
  }
`

export const adsPerHour = styled.div`
  margin-top: 20px;
  font-weight: 600;
  font-size: 22px;
  line-height: 20px;
  color: var(--brave-palette-neutral700);
`

export const optionBar = styled.div`
  position: relative;
  margin: 23px auto 31px;
  width: 238px;
  background: #E4DBFF;
  border-radius: 100px;
  height: 8px;
  display: flex;
  align-items: center;
  overflow: visible;

  button {
    ${mixins.buttonReset}
    flex: 2 0 auto;
    padding: 2px;
    text-align: center;
    cursor: pointer;
    display: flex;
    justify-content: center;

    &:first-of-type {
      flex: 1 0 auto;
      justify-content: flex-start;
    }

    &:last-of-type {
      flex: 1 0 auto;
      justify-content: flex-end;
    }
  }
`

export const optionMarker = styled.div`
  height: 4px;
  width: 4px;
  border-radius: 50%;
  background: #F8F9FA;
`

export const optionHandle = styled.div`
  position: absolute;
  height: 24px;
  width: 24px;
  top: -8px;
  left: calc(var(--optionbar-handle-position, 0px) - 12px);
  border-radius: 50%;
  background: #845EF7;
`
