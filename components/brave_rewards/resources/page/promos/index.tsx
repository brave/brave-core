/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import tapBg from './assets/tap-bg.svg'
import { StyledLink, StyledInfo } from '../../ui/components/sidebarPromo/style'

const openTapNetwork = () => {
  window.open('https://brave.tapnetwork.io', '_blank')
}

export type PromoType = 'tap-network'

export interface Promo {
  title: string,
  imagePath: string,
  copy: JSX.Element,
  disclaimer?: string,
  supportedLocales: string[]
}

export const activePromos = [
  'tap-network'
]

export const getPromo = (type: PromoType, getLocale: any) => {
  switch (type) {
    case 'tap-network':
      return {
        imagePath: tapBg,
        copy: (
          <div>
            <StyledInfo>
              {getLocale('tapNetworkInfo')}
            </StyledInfo>
            <StyledLink onClick={openTapNetwork}>
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
