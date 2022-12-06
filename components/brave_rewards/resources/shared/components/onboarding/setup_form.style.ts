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

export const slider = styled.div`
  margin: 23px auto 31px;
  width: 238px;

  --slider-bar-height: 8px;
  --slider-handle-height: 24px;
  --slider-handle-width: 24px;

  .slider {
    cursor: pointer;
  }

  .slider-bar {
    background: #E4DBFF;
    border-radius: 100px;
    padding: 0 2px;
  }

  .slider-marker {
    background: #F8F9FA;
    border-radius: 50%;
    height: 4px;
    width: 4px;
  }

  .slider-handle {
    ${mixins.buttonReset}
    border-radius: 50%;
    background: #845EF7;
    cursor: pointer;
  }
`
