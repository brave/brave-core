/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { PlatformContext } from '../lib/platform_context'
import { LocaleContext } from '../../shared/lib/locale_context'
import { useActions, useRewardsData } from '../lib/redux_hooks'
import { BitflyerPromotion } from './bitflyer_promotion'
import { externalWalletFromExtensionData } from '../../shared/lib/external_wallet'
import { PromotionKey, getAvailablePromotions, getPromotionURL } from '../lib/promotions'
import { CloseIcon } from '../../shared/components/icons/close_icon'

import * as style from './sidebar_promotion_panel.style'

interface PromotionMessages {
  title: string
  text1: string
  text2?: string
  disclaimer?: string
}

function getPromotionMessages (
  key: Exclude<PromotionKey, 'bitflyer-verification'>
): PromotionMessages {
  switch (key) {
    case 'brave-creators':
      return {
        title: 'braveCreatorsPromoTitle',
        text1: 'braveCreatorsPromoInfo1',
        text2: 'braveCreatorsPromoInfo2'
      }
    case 'tap-network':
      return {
        title: 'tapNetworkTitle',
        text1: 'tapNetworkInfo',
        disclaimer: 'tapNetworkDisclaimer'
      }
    case 'uphold-card':
      return {
        title: 'upholdPromoTitle',
        text1: 'upholdPromoInfo'
      }
  }
}

export function SidebarPromotionPanel () {
  const { isAndroid } = React.useContext(PlatformContext)
  const { getString } = React.useContext(LocaleContext)

  const actions = useActions()

  const {
    currentCountryCode,
    promosDismissed,
    externalWallet
  } = useRewardsData((data) => ({
    currentCountryCode: data.currentCountryCode,
    promosDismissed: data.ui.promosDismissed || {},
    externalWallet: data.externalWallet
  }))

  return (
    <>
      {
        getAvailablePromotions(
          currentCountryCode,
          externalWalletFromExtensionData(externalWallet),
          isAndroid
        ).map((key) => {
          if (promosDismissed[key]) {
            return null
          }

          const visitPromotionURL = () => {
            window.open(getPromotionURL(key), '_blank', 'noreferrer')
          }

          const onDismiss = () => {
            actions.dismissPromoPrompt(key)
          }

          const onClose = (event: React.UIEvent) => {
            onDismiss()
            event.stopPropagation()
          }

          if (key === 'bitflyer-verification') {
            return (
              <BitflyerPromotion
                key={key}
                onLearnMore={visitPromotionURL}
                onDismiss={onDismiss}
              />
            )
          }

          const messages = getPromotionMessages(key)

          return (
            <style.promotion
              key={key}
              className={`promotion-${key}`}
              onClick={visitPromotionURL}
            >
              <style.header>
                <style.title>
                  {getString(messages.title)}
                </style.title>
                <style.close>
                  <button onClick={onClose}><CloseIcon /></button>
                </style.close>
              </style.header>
              <style.text>
                {getString(messages.text1)}
              </style.text>
              {
                messages.text2 &&
                  <style.text>
                    {getString(messages.text2)}
                  </style.text>
              }
              {
                messages.disclaimer &&
                  <style.disclaimer>
                    {getString(messages.disclaimer)}
                  </style.disclaimer>
              }
            </style.promotion>
          )
        })
      }
    </>
  )
}
