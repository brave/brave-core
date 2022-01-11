/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as knobs from '@storybook/addon-knobs'

import { LocaleContext } from '../../../lib/locale_context'
import { ExternalWallet } from '../../../lib/external_wallet'
import { WithThemeVariables } from '../../with_theme_variables'
import { WalletCard } from '../'

import { localeStrings } from './locale_strings'

const locale = {
  getString (key: string) {
    return localeStrings[key] || 'MISSING'
  }
}

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
    status: 'verified',
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
            balance={0}
            externalWallet={externalWallet}
            earningsThisMonth={0}
            earningsLastMonth={1}
            nextPaymentDate={nextPaymentDate.getTime()}
            exchangeRate={0.75}
            exchangeCurrency={'USD'}
            showSummary={knobs.boolean('Show Summary', true)}
            summaryData={summaryData}
            autoContributeEnabled={true}
            onExternalWalletAction={actionLogger('onExternalWalletAction')}
            onViewPendingTips={actionLogger('onViewPendingTips')}
            onViewStatement={actionLogger('onViewStatement')}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}
