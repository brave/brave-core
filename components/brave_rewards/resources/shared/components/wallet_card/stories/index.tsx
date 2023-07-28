/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as knobs from '@storybook/addon-knobs'

import { LocaleContext, createLocaleContextForTesting } from '../../../lib/locale_context'
import { ExternalWallet } from '../../../lib/external_wallet'
import { WithThemeVariables } from '../../with_theme_variables'
import { WalletCard } from '../'

import { localeStrings } from './locale_strings'
import * as mojom from '../../../../shared/lib/mojom'
import { optional } from '../../../../shared/lib/optional'

const locale = createLocaleContextForTesting(localeStrings)

export default {
  title: 'Rewards'
}

function actionLogger (name: string) {
  return (...args: any[]) => {
    console.log(name, ...args)
  }
}

function getNextPaymentDate (days: number) {
  const now = new Date()
  return new Date(now.getFullYear(), now.getMonth(), now.getDate() + days)
}

export function Wallet () {
  const summaryData = {
    adEarnings: knobs.boolean('Ad Earnings Received', true) ? 10 : 0,
    autoContributions: 10,
    oneTimeTips: -2,
    monthlyTips: -19,
    pendingTips: 8
  }

  const externalWallet: ExternalWallet = {
    provider: 'uphold',
    status: knobs.boolean('Wallet disconnected', false)
      ? mojom.WalletStatus.kLoggedOut
      : mojom.WalletStatus.kConnected,
    username: 'brave123',
    links: {}
  }

  const nextPaymentDate = getNextPaymentDate(
    knobs.number('Days Until Payment', 20))

  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <div style={{ width: '375px' }}>
          <WalletCard
            balance={optional(0)}
            isGrandfatheredUser={false}
            externalWallet={externalWallet && null}
            providerPayoutStatus={'complete'}
            minEarningsThisMonth={0.25}
            maxEarningsThisMonth={0.75}
            minEarningsLastMonth={1}
            maxEarningsLastMonth={1.5}
            nextPaymentDate={nextPaymentDate.getTime()}
            exchangeRate={0.75}
            exchangeCurrency={'USD'}
            showSummary={knobs.boolean('Show Summary', true)}
            summaryData={summaryData}
            autoContributeEnabled={true}
            onExternalWalletAction={actionLogger('onExternalWalletAction')}
            onViewStatement={actionLogger('onViewStatement')}
            onManageAds={actionLogger('onManageAds')}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}
