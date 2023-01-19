/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWallet } from '../../shared/lib/external_wallet'

export type PromotionKey =
  'bitflyer-verification' |
  'brave-creators' |
  'gemini' |
  'tap-network' |
  'uphold-card'

function getSupportedLocales (key: Exclude<PromotionKey, 'brave-creators'>) {
  switch (key) {
    case 'bitflyer-verification':
      return ['JP']
    case 'gemini':
      return ['US']
    case 'tap-network':
      return ['US']
    case 'uphold-card':
      return ['US']
  }
}

function isSupportedLocale (key: PromotionKey, countryCode: string) {
  // This promotion is valid in all locales
  if (key === 'brave-creators') {
    return true
  }

  return getSupportedLocales(key).includes(countryCode)
}

export function getPromotionURL (key: PromotionKey) {
  switch (key) {
    case 'bitflyer-verification':
      return 'https://support.brave.com/hc/en-us/articles/4403459972365-%E5%BA%83%E5%91%8A%E9%96%B2%E8%A6%A7%E5%A0%B1%E9%85%AC%E3%81%A7%E7%8D%B2%E5%BE%97%E3%81%97%E3%81%9FBAT%E3%81%AF%E5%BC%95%E3%81%8D%E5%87%BA%E3%81%99%E3%81%93%E3%81%A8%E3%81%8C%E3%81%A7%E3%81%8D%E3%81%BE%E3%81%99%E3%81%8B'
    case 'brave-creators':
      return 'https://creators.brave.com'
    case 'gemini':
      return 'https://www.gemini.com/brave'
    case 'tap-network':
      return 'https://brave.tapnetwork.io'
    case 'uphold-card':
      return 'https://uphold.com/brave/upholdcard'
  }
}

function getPromotions (
  externalWallet: ExternalWallet | null,
  isAndroid: boolean
) {
  const list: PromotionKey[] = []

  if (isAndroid) {
    if (!externalWallet) {
      list.unshift('bitflyer-verification')
    }
  } else {
    list.unshift('tap-network')
    if (externalWallet && externalWallet.provider === 'uphold') {
      list.unshift('uphold-card')
    }
    list.unshift('gemini')
    list.unshift('brave-creators')
  }

  return list
}

export function getAvailablePromotions (
  countryCode: string,
  externalWallet: ExternalWallet | null,
  isAndroid: boolean
) {
  return getPromotions(externalWallet, isAndroid).filter((key) => {
    return isSupportedLocale(key, countryCode)
  })
}
