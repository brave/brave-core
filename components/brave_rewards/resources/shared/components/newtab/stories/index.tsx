/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as knobs from '@storybook/addon-knobs'

import { LocaleContext, createLocaleContextForTesting } from '../../../lib/locale_context'
import { WithThemeVariables } from '../../with_theme_variables'
import { RewardsCard } from '../rewards_card'

import { localeStrings } from './locale_strings'
import * as mojom from '../../../../shared/lib/mojom'
import { optional } from '../../../../shared/lib/optional'

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
            userType='unconnected'
            vbatDeadline={Date.parse('2023-01-01T00:00:00-05:00')}
            isUnsupportedRegion={false}
            declaredCountry='US'
            needsBrowserUpgradeToServeAds={false}
            rewardsBalance={optional(91.5812)}
            exchangeCurrency='USD'
            exchangeRate={0.82}
            providerPayoutStatus={'off'}
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
              links: {}
            } : null}
            nextPaymentDate={nextPaymentDate}
            minEarningsThisMonth={0.142}
            maxEarningsThisMonth={1.142}
            minEarningsLastMonth={1.00}
            maxEarningsLastMonth={1.25}
            contributionsThisMonth={10}
            publishersVisited={4}
            canConnectAccount={true}
            showSelfCustodyInvite={true}
            onEnableRewards={actionLogger('onEnableRewards')}
            onSelectCountry={actionLogger('onSelectCountry')}
            onClaimGrant={actionLogger('onClaimGrant')}
            onSelfCustodyInviteDismissed={
              actionLogger('onSelfCustodyInviteDismissed')
            }
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}
