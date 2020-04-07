/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import tapBg from './assets/tap_bg.svg'
import p2pBg from './assets/p2p_bg.svg'
import { StyledLink, StyledInfo, Linkp2p } from '../../ui/components/sidebarPromo/style'
import { getLocale } from '../../../../common/locale'

export type PromoType = 'tap-network' | 'p2p'

export interface Promo {
  title: string,
  imagePath: string,
  copy: JSX.Element,
  disclaimer?: string,
  supportedLocales: string[]
}

export const getActivePromos = (rewardsData: Rewards.State) => {
  let promos = ['tap-network']

  if (rewardsData) {
    let wallet = rewardsData.externalWallet
    if (wallet && wallet.address.length > 0 && wallet.status === 2) { // WalletStatus::VERIFIED
      promos.push('p2p')
    }
  }

  return promos
}

const openLink = (type: PromoType, rewardsData: Rewards.State) => {
  let url
  switch (type) {
    case 'p2p': {
      if (!rewardsData || !rewardsData.externalWallet) {
        url = 'https://referrers.brave.com'
        break
      }

      url = `https://referrers.brave.com/uphold_connections/login?locale=en&uphold_card_id=${rewardsData.externalWallet.address}`
      break
    }
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
    case 'p2p':
      return {
        imagePath: p2pBg,
        copy: (
          <>
            <Linkp2p onClick={link}>
              {getLocale('p2pLink')}
            </Linkp2p>
          </>
        ),
        supportedLocales: [],
        title: getLocale('p2pTitle'),
        disclaimer: getLocale('p2pDisclaimer')
      }
    default:
      return null
  }
}
