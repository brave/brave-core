/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { RewardsTourPromo } from '../../shared/components/onboarding'
import { getActivePromos, getPromo } from './promos'
import { SidebarPromo } from '../../ui/components'

import * as style from './sidebar_promotion_panel.style'

interface Props {
  onTakeRewardsTour: () => void
}

export function SidebarPromotionPanel (props: Props) {
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
    promosDismissed: data.ui.promosDismissed,
    externalWallet: data.externalWallet
  }))

  const renderOnboardingPromo = () => {
    const promoKey = 'rewards-tour'

    if (showOnboarding ||
        adsData && adsData.adsEnabled ||
        promosDismissed && promosDismissed[promoKey]) {
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
        getActivePromos(externalWallet).map((key) => {
          if (promosDismissed && promosDismissed[key]) {
            return null
          }

          const promo = getPromo(key)
          if (!promo || !promo.supportedLocales.includes(currentCountryCode)) {
            return null
          }

          const onDismiss = (event: React.UIEvent) => {
            actions.dismissPromoPrompt(key)
            event.preventDefault()
          }

          return (
            <SidebarPromo
              key={`${key}-promo`}
              {...promo}
              onDismissPromo={onDismiss}
            />
          )
        })
      }
    </div>
  )
}
