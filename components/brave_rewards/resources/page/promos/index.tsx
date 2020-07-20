/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import tapBg from './assets/tap_bg.svg'
import upholdBg from './assets/uphold_bg.png'
import { StyledInfo } from '../../ui/components/sidebarPromo/style'
import { getLocale } from '../../../../common/locale'

export type PromoType = 'uphold' | 'tap-network'

export interface Promo {
  title: string
  imagePath: string
  link: string
  copy: JSX.Element
  disclaimer?: string
  supportedLocales: string[]
}

export const getActivePromos = (rewardsData: Rewards.State) => {
  let promos = ['tap-network']

  if (rewardsData) {
    let wallet = rewardsData.externalWallet
    if (wallet && wallet.address.length > 0 && wallet.status === 2) { // WalletStatus::VERIFIED
      promos.unshift('uphold')
    }
  }

  return promos
}

const getLink = (type: PromoType) => {
  switch (type) {
    case 'tap-network': {
      return 'https://brave.tapnetwork.io'
    }
    case 'uphold': {
      return 'https://uphold.com/brave/upholdcard'
    }
  }

  return ''
}

export const getPromo = (type: PromoType, rewardsData: Rewards.State) => {
  switch (type) {
    case 'tap-network':
      return {
        imagePath: tapBg,
        link: getLink(type),
        copy: (
          <StyledInfo>
            {getLocale('tapNetworkInfo')}
          </StyledInfo>
        ),
        supportedLocales: ['US'],
        title: getLocale('tapNetworkTitle'),
        disclaimer: getLocale('tapNetworkDisclaimer')
      }
    case 'uphold':
      return {
        imagePath: upholdBg,
        link: getLink(type),
        copy: (
          <StyledInfo>
            {getLocale('upholdPromoInfo')}
          </StyledInfo>
        ),
        title: getLocale('upholdPromoTitle')
      }
    default:
      return null
  }
}
