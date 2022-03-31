/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'

import { MoneyBagIcon } from './icons/money_bag_icon'

import confettiBackgroundImage from '../assets/grant_confetti.svg'

const style = {
  graphic: styled.div`
    background: center no-repeat url('${confettiBackgroundImage}');
    background-size: auto 52px;

    display: flex;
    justify-content: center;

    > :first-child {
      margin-top: 3px;
      padding: 13px;
      border-radius: 50%;
      background: rgba(97, 195, 225, 0.15);

      .icon {
        color: #61C3E1;
        height: 27px;
        width: auto;
        vertical-align: middle;
      }
    }
  `
}

export function GrantAvailableGraphic () {
  return (
    <style.graphic>
      <div><MoneyBagIcon /></div>
    </style.graphic>
  )
}
