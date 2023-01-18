/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as knobs from '@storybook/addon-knobs'

import { LocaleContext, createLocaleContextForTesting } from '../../../lib/locale_context'
import { WithThemeVariables } from '../../with_theme_variables'
import { RewardsCard } from '../rewards_card'
import { SponsoredImageTooltip } from '../sponsored_image_tooltip'

import { localeStrings } from './locale_strings'
import * as mojom from '../../../../shared/lib/mojom'

const localeContext = createLocaleContextForTesting(localeStrings)

export default {
  title: 'Rewards/New Tab'
}

function actionLogger (name: string, ...args: any[]) {
  return (...args: any[]) => console.log(name, ...args)
}

export function Card () {
  const daysUntilPayment = knobs.number('Days Until Payment', 20)
  const nextPaymentDate = Date.now() + 1000 * 60 * 60 * 24 * daysUntilPayment
  const showGrant = knobs.boolean('Grant Available', false)
  const disconnectedWallet = knobs.boolean('Disconnected', false)

  return (
    <LocaleContext.Provider value={localeContext}>
      <WithThemeVariables>
        <div style={{ width: '284px' }}>
          <RewardsCard
            rewardsEnabled={true}
            userType='connected'
            vbatDeadline={Date.parse('2023-01-01T00:00:00-05:00')}
            isUnsupportedRegion={false}
            declaredCountry='US'
            adsEnabled={true}
            adsSupported={true}
            needsBrowserUpgradeToServeAds={false}
            rewardsBalance={91.5812}
            exchangeCurrency='USD'
            exchangeRate={0.82}
            providerPayoutStatus={'complete'}
            grantInfo={showGrant ? {
              id: '',
              amount: 0.15,
              type: 'ads',
              createdAt: Date.now(),
              claimableUntil: Date.now() + 1000 * 10 * 24 * 60 * 60,
              expiresAt: Date.now() + 1000 * 10 * 24 * 60 * 60
            } : null}
            externalWallet={disconnectedWallet ? {
              provider: 'uphold',
              status: mojom.WalletStatus.kLoggedOut,
              username: '',
              links: {
                reconnect: 'https://brave.com'
              }
            } : null}
            nextPaymentDate={nextPaymentDate}
            earningsThisMonth={0.142}
            earningsLastMonth={1.25}
            contributionsThisMonth={10}
            publishersVisited={4}
            canConnectAccount={true}
            onEnableRewards={actionLogger('onEnableRewards')}
            onEnableAds={actionLogger('onEnableAds')}
            onSelectCountry={actionLogger('onSelectCountry')}
            onClaimGrant={actionLogger('onClaimGrant')}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

export function SponsoredImage () {
  return (
    <LocaleContext.Provider value={localeContext}>
      <WithThemeVariables>
        <div style={{ width: '284px' }}>
          <SponsoredImageTooltip
            adsEnabled={false}
            onEnableAds={actionLogger('onEnableAds')}
            onClose={actionLogger('onClose')}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}
