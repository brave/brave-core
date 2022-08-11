/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { PromotionKey } from '../lib/promotions'
import * as mixins from '../../shared/lib/css_mixins'

import geminiBackground from '../assets/gemini_bg.svg'
import tapBackgroud from '../assets/tap_bg.svg'
import upholdCardBackground from '../assets/uphold_card_bg.png'
import upholdEquitiesBackground from '../assets/uphold_equities_bg.svg'

function getBackgroundImage (key: PromotionKey) {
  switch (key) {
    case 'bitflyer-verification': return ''
    case 'gemini': return geminiBackground
    case 'tap-network': return tapBackgroud
    case 'uphold-card': return upholdCardBackground
    case 'uphold-equities': return upholdEquitiesBackground
  }
}

export const rewardsTourPromo = styled.div`
  margin-top: 30px;
`

export const promotion = styled.div`
  margin-top: 30px;
  color: #000;
  background: #fff;
  border-radius: 8px;
  min-height: 157px;
  cursor: pointer;
  padding-left: 132px;
  padding-bottom: 20px;

  background-position: top left;
  background-size: contain;
  background-repeat: no-repeat;

  &.promotion-gemini {
    background-image: url(/${getBackgroundImage('gemini')});
  }

  &.promotion-tap-network {
    background-image: url(/${getBackgroundImage('tap-network')});
  }

  &.promotion-uphold-card {
    background-image: url(/${getBackgroundImage('uphold-card')});
  }

  &.promotion-uphold-equities {
    background-image: url(/${getBackgroundImage('uphold-equities')});
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
