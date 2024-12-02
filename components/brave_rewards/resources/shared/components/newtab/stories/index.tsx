/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'
import * as knobs from '@storybook/addon-knobs'

import { LocaleContext, createLocaleContextForTesting } from '../../../lib/locale_context'
import { RewardsCard } from '../rewards_card'

import { localeStrings } from './locale_strings'
import { optional } from '../../../../shared/lib/optional'

const localeContext = createLocaleContextForTesting(localeStrings)

export default {
  title: 'Rewards/New Tab'
}

function actionLogger (name: string, ...args: any[]) {
  return (...args: any[]) => console.log(name, ...args)
}

const style = {
  card: styled.div`
    width: 284px;
    background: #1C1E26B2;
    backdrop-filter: blur(27.5px);
    border-radius: 16px;
    padding: 24px;
  `
}

export function Card () {
  const daysUntilPayment = knobs.number('Days Until Payment', 20)
  const nextPaymentDate = Date.now() + 1000 * 60 * 60 * 24 * daysUntilPayment
  const disconnectedWallet = knobs.boolean('Disconnected', false)
  const showSelfCustodyInvite = knobs.boolean('Show Self-Custody Invite', false)
  const tosUpdateRequired = knobs.boolean('TOS Update Required', false)
  const payoutStatus =
    knobs.select('Payout Status', ['off', 'processing', 'complete'], 'off')

  return (
    <LocaleContext.Provider value={localeContext}>
      <style.card>
        <RewardsCard
          rewardsEnabled={true}
          userType='connected'
          declaredCountry='US'
          needsBrowserUpgradeToServeAds={false}
          rewardsBalance={optional(91.5812)}
          exchangeCurrency='USD'
          exchangeRate={0.82}
          providerPayoutStatus={payoutStatus}
          externalWallet={{
            provider: 'uphold',
            authenticated: !disconnectedWallet,
            name: '',
            url: ''
          }}
          nextPaymentDate={nextPaymentDate}
          adsReceivedThisMonth={2}
          minEarningsThisMonth={0.142}
          maxEarningsThisMonth={1.142}
          minEarningsLastMonth={1.00}
          maxEarningsLastMonth={1.25}
          contributionsThisMonth={10}
          publishersVisited={4}
          showSelfCustodyInvite={showSelfCustodyInvite}
          isTermsOfServiceUpdateRequired={tosUpdateRequired}
          onEnableRewards={actionLogger('onEnableRewards')}
          onSelectCountry={actionLogger('onSelectCountry')}
          onSelfCustodyInviteDismissed={
            actionLogger('onSelfCustodyInviteDismissed')
          }
          onTermsOfServiceUpdateAccepted={
            actionLogger('onTermsOfServiceUpdateAgreed')
          }
        />
      </style.card>
    </LocaleContext.Provider>
  )
}
