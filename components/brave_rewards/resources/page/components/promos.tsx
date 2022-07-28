/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import geminiBg from '../assets/gemini_bg.svg'
import tapBg from '../assets/tap_bg.svg'
import upholdCardBg from '../assets/uphold_card_bg.png'
import upholdEquitiesBg from '../assets/uphold_equities_bg.svg'

import { StyledInfo } from '../../ui/components/sidebarPromo/style'
import { getLocale } from '../../../../common/locale'

export type PromoType = 'bitflyer-verification' | 'gemini' | 'tap-network' | 'uphold-card' | 'uphold-equities'

export interface Promo {
  title: string
  imagePath: string
  link: string
  copy: React.ReactNode
  disclaimer?: string
  supportedLocales: string[]
}

export function getActivePromos (
  wallet: Rewards.ExternalWallet|undefined,
  isMobile: boolean = false
) {
  const promos: PromoType[] = []

  if (!wallet) {
    return promos
  }

  if (isMobile) {
    if (wallet.type === 'bitflyer') {
      promos.unshift('bitflyer-verification')
    }
  } else {
    if (wallet.type === 'uphold') {
      promos.unshift('tap-network')
      if (wallet.status === 2 && wallet.address) { // WalletStatus::VERIFIED
        promos.unshift('uphold-card')
      }
      promos.unshift('uphold-equities')
    }
    promos.unshift('gemini')
  }

  return promos
}

const getLink = (type: PromoType) => {
  switch (type) {
    case 'bitflyer-verification': {
      return 'https://support.brave.com/hc/en-us/articles/4403459972365-%E5%BA%83%E5%91%8A%E9%96%B2%E8%A6%A7%E5%A0%B1%E9%85%AC%E3%81%A7%E7%8D%B2%E5%BE%97%E3%81%97%E3%81%9FBAT%E3%81%AF%E5%BC%95%E3%81%8D%E5%87%BA%E3%81%99%E3%81%93%E3%81%A8%E3%81%8C%E3%81%A7%E3%81%8D%E3%81%BE%E3%81%99%E3%81%8B'
    }
    case 'gemini': {
      return 'https://www.gemini.com/brave'
    }
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

// Ensure that images are retrieved from the root path, since we may be on a
// rewards page subpath like brave://rewards/uphold.
// TODO: Come up with a more generic solution for this, since it affects other
// resource loads
const getRootImagePath = (path: string) => {
  return `/${path}`
}

export function getPromo (type: PromoType): Promo|null {
  switch (type) {
    case 'bitflyer-verification':
      return {
        imagePath: '',
        link: getLink(type),
        copy: getLocale('bitflyerVerificationPromoInfo'),
        supportedLocales: ['JP'],
        title: getLocale('bitflyerVerificationPromoTitle')
      }
    case 'gemini':
      return {
        imagePath: getRootImagePath(geminiBg),
        link: getLink(type),
        copy: (
          <StyledInfo>
            {getLocale('geminiPromoInfo')}
          </StyledInfo>
        ),
        supportedLocales: [
          'AR', 'AT', 'AU', 'BE', 'BG', 'BM', 'BR', 'BS', 'BT', 'CA', 'CH', 'CL', 'CY', 'CZ', 'DK', 'EE', 'EG', 'ES',
          'FI', 'GB', 'GG', 'GI', 'GR', 'HK', 'HR', 'HU', 'IL', 'IN', 'IS', 'IT', 'JE', 'KR', 'KY', 'LI', 'LT', 'LU',
          'LV', 'MM', 'MT', 'NG', 'NL', 'NO', 'NZ', 'PE', 'PH', 'PL', 'PT', 'RO', 'SE', 'SG', 'SI', 'SK', 'TR', 'TW',
          'US', 'UY', 'VC', 'VG', 'VN', 'ZA'
        ],
        title: getLocale('geminiPromoTitle')
      }
    case 'tap-network':
      return {
        imagePath: getRootImagePath(tapBg),
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
        imagePath: getRootImagePath(upholdCardBg),
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
        imagePath: getRootImagePath(upholdEquitiesBg),
        link: getLink(type),
        copy: (
          <StyledInfo>
            {getLocale('upholdPromoEquitiesInfo')}
          </StyledInfo>
        ),
        supportedLocales: [
          'AF', 'AG', 'AI', 'AN', 'AO', 'AR', 'AW', 'AZ', 'BB', 'BD', 'BF', 'BH', 'BI', 'BJ', 'BL', 'BM', 'BN', 'BO',
          'BR', 'BS', 'BT', 'BW', 'BZ', 'CF', 'CI', 'CK', 'CL', 'CM', 'CN', 'CO', 'CR', 'CV', 'DJ', 'DM', 'DO', 'DZ',
          'EC', 'EH', 'ER', 'ET', 'FJ', 'FK', 'FM', 'GA', 'GD', 'GE', 'GH', 'GM', 'GN', 'GQ', 'GS', 'GT', 'GW', 'GY',
          'HK', 'HN', 'HT', 'ID', 'IN', 'IO', 'JM', 'JO', 'KE', 'KG', 'KH', 'KI', 'KM', 'KN', 'KR', 'KW', 'KY', 'KZ',
          'LA', 'LB', 'LC', 'LK', 'LR', 'LS', 'LY', 'MA', 'MG', 'MH', 'ML', 'MN', 'MO', 'MR', 'MS', 'MU', 'MV', 'MW',
          'MZ', 'NA', 'NE', 'NG', 'NI', 'NP', 'NR', 'NU', 'NZ', 'OM', 'PA', 'PE', 'PG', 'PH', 'PK', 'PN', 'PS', 'PW',
          'PY', 'QA', 'RW', 'SA', 'SB', 'SC', 'SH', 'SL', 'SN', 'SO', 'SR', 'ST', 'SV', 'SZ', 'TC', 'TD', 'TG', 'TH',
          'TJ', 'TK', 'TL', 'TM', 'TN', 'TO', 'TR', 'TT', 'TV', 'TW', 'TZ', 'UG', 'UY', 'UZ', 'VC', 'VE', 'VG', 'VN',
          'VU', 'WS', 'ZM', 'ZW'
        ],
        title: getLocale('upholdPromoEquitiesTitle')
      }
    default:
      return null
  }
}
