/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { PromotionKey } from '../lib/promotions'
import * as mixins from '../../shared/lib/css_mixins'

import braveCreatorsBackground from '../assets/brave_creators_bg.svg'
import tapBackgroud from '../assets/tap_bg.svg'
import upholdCardBackground from '../assets/uphold_card_bg.png'

function getBackgroundImage (key: PromotionKey) {
  switch (key) {
    case 'bitflyer-verification': return ''
    case 'brave-creators': return braveCreatorsBackground
    case 'tap-network': return tapBackgroud
    case 'uphold-card': return upholdCardBackground
  }
}

export const promotion = styled.div`
  color: #000;
  background: #fff;
  box-shadow:
    0px 0px 1px rgba(0, 0, 0, 0.11),
    0px 0.5px 1.5px rgba(0, 0, 0, 0.1);
  border-radius: 8px;
  min-height: 157px;
  cursor: pointer;
  padding-left: 132px;
  padding-bottom: 20px;

  background-position: top left;
  background-size: contain;
  background-repeat: no-repeat;

  &.promotion-brave-creators {
    background-image: url(${getBackgroundImage('brave-creators')});
  }

  &.promotion-tap-network {
    background-image: url(${getBackgroundImage('tap-network')});
  }

  &.promotion-uphold-card {
    background-image: url(${getBackgroundImage('uphold-card')});
  }
`

export const header = styled.div`
  font-size: 15px;
  line-height: 1.6;
  font-weight: 600;
  display: flex;
`

export const title = styled.div`
  flex: 1 1 auto;
  margin-top: 14px;
`

export const close = styled.div`
  flex: 0 1 auto;
  padding: 6px;

  button {
    ${mixins.buttonReset}
    padding: 5px;
    cursor: pointer;
  }

  .icon {
    color: #AEB1C2;
    display: block;
    height: 12px;
    width: auto;
  }
`

export const text = styled.div`
  margin: 9px 16px 0 0;
  color: #84889c;
  font-size: 11.5px;
  line-height: 1.57;
`

export const disclaimer = styled.div`
  margin: 10px 16px 0 0;
  color: #AEB1C2;
  font-size: 11px;
  font-weight: 600;
`
