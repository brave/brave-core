/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import tapBg from './assets/tap_bg.svg'
import { StyledLink, StyledInfo } from '../../ui/components/sidebarPromo/style'
import { getLocale } from '../../../../common/locale'

export type PromoType = 'tap-network'

export interface Promo {
  title: string,
  imagePath: string,
  copy: JSX.Element,
  disclaimer?: string,
  supportedLocales: string[]
}

export const getActivePromos = (rewardsData: Rewards.State) => {
  let promos = ['tap-network']

  return promos
}

const openLink = (type: PromoType, rewardsData: Rewards.State) => {
  let url
  switch (type) {
    case 'tap-network': {
      url = 'https://brave.tapnetwork.io'
      break
    }
  }

  window.open(url, '_blank')
}

export const getPromo = (type: PromoType, rewardsData: Rewards.State) => {
  const link = openLink.bind(this, type, rewardsData)
  switch (type) {
    case 'tap-network':
      return {
        imagePath: tapBg,
        copy: (
          <div>
            <StyledInfo>
              {getLocale('tapNetworkInfo')}
            </StyledInfo>
            <StyledLink onClick={link}>
              {getLocale('tapNetworkLink')}
            </StyledLink>
          </div>
        ),
        supportedLocales: ['US'],
        title: getLocale('tapNetworkTitle'),
        disclaimer: getLocale('tapNetworkDisclaimer')
      }
    default:
      return null
  }
}
