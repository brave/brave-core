/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { PlatformContext } from '../lib/platform_context'
import { LocaleContext } from '../../shared/lib/locale_context'
import { useActions, useRewardsData } from '../lib/redux_hooks'
import { RewardsTourPromo } from '../../shared/components/onboarding'
import { BitflyerPromotion } from './bitflyer_promotion'
import { externalWalletFromExtensionData } from '../../shared/lib/external_wallet'
import { PromotionKey, getAvailablePromotions, getPromotionURL } from '../lib/promotions'
import { CloseIcon } from '../../shared/components/icons/close_icon'

import * as style from './sidebar_promotion_panel.style'

interface PromotionMessages {
  title: string
  text: string
  disclaimer: string
}

function getPromotionMessages (
  key: Exclude<PromotionKey, 'bitflyer-verification'>
): PromotionMessages {
  switch (key) {
    case 'gemini':
      return {
        title: 'geminiPromoTitle',
        text: 'geminiPromoInfo',
        disclaimer: ''
      }
    case 'tap-network':
      return {
        title: 'tapNetworkTitle',
        text: 'tapNetworkInfo',
        disclaimer: 'tapNetworkDisclaimer'
      }
    case 'uphold-card':
      return {
        title: 'upholdPromoTitle',
        text: 'upholdPromoInfo',
        disclaimer: ''
      }
    case 'uphold-equities':
      return {
        title: 'upholdPromoEquitiesTitle',
        text: 'upholdPromoEquitiesInfo',
        disclaimer: ''
      }
  }
}

interface Props {
  onTakeRewardsTour: () => void
}

export function SidebarPromotionPanel (props: Props) {
  const { isAndroid } = React.useContext(PlatformContext)
  const { getString } = React.useContext(LocaleContext)

  const actions = useActions()

  const {
    adsData,
    currentCountryCode,
    showOnboarding,
    promosDismissed,
    externalWallet
  } = useRewardsData((data) => ({
    adsData: data.adsData,
    currentCountryCode: data.currentCountryCode,
    showOnboarding: data.showOnboarding,
    promosDismissed: data.ui.promosDismissed || {},
    externalWallet: data.externalWallet
  }))

  const renderOnboardingPromo = () => {
    const promoKey = 'rewards-tour'

    if (showOnboarding ||
        adsData && adsData.adsEnabled ||
        promosDismissed[promoKey]) {
      return null
    }

    const onClose = () => {
      actions.dismissPromoPrompt(promoKey)
    }

    return (
      <style.rewardsTourPromo>
        <RewardsTourPromo
          onTakeTour={props.onTakeRewardsTour}
          onClose={onClose}
        />
      </style.rewardsTourPromo>
    )
  }

  return (
    <div>
      {renderOnboardingPromo()}
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
            window.open(getPromotionURL(key), '_blank')
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
                {getString(messages.text)}
              </style.text>
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
    </div>
  )
}
