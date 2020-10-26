/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import tapBg from './assets/tap_bg.svg'
import upholdCardBg from './assets/uphold_card_bg.png'
import upholdEquitiesBg from './assets/uphold_equities_bg.svg'
import { StyledInfo } from '../../ui/components/sidebarPromo/style'
import { getLocale } from '../../../../common/locale'

export type PromoType = 'uphold-card' | 'tap-network' | 'uphold-equities'

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
    if (wallet && wallet.status === 2 && wallet.address && wallet.address.length > 0) { // WalletStatus::VERIFIED
      promos.unshift('uphold-card')
    }
  }

  promos.unshift('uphold-equities')

  return promos
}

const getLink = (type: PromoType) => {
  switch (type) {
    case 'tap-network': {
      return 'https://brave.tapnetwork.io'
    }
    case 'uphold-card': {
      return 'https://uphold.com/brave/upholdcard'
    }
    case 'uphold-equities': {
      return 'https://uphold.com/en/buy-fractional-shares/brave'
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
    case 'uphold-card':
      return {
        imagePath: upholdCardBg,
        link: getLink(type),
        copy: (
          <StyledInfo>
            {getLocale('upholdPromoInfo')}
          </StyledInfo>
        ),
        supportedLocales: ['US'],
        title: getLocale('upholdPromoTitle')
      }
    case 'uphold-equities':
      return {
        imagePath: upholdEquitiesBg,
        link: getLink(type),
        copy: (
          <StyledInfo>
            {getLocale('upholdPromoEquitiesInfo')}
          </StyledInfo>
        ),
        supportedLocales: ['US'],
        title: getLocale('upholdPromoEquitiesTitle')
      }
    default:
      return null
  }
}
