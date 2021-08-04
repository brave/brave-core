/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Host, HostState } from '../lib/interfaces'
import { localeStrings } from './locale_strings'
import { createStateManager } from '../../shared/lib/state_manager'

import { App } from '../components/app'

export default {
  title: 'Rewards'
}

function createHost (): Host {
  const stateManager = createStateManager<HostState>({
    hidePublisherUnverifiedNote: false,
    balance: 10.2,
    exchangeInfo: {
      rate: 0.75,
      currency: 'USD'
    },
    earningsInfo: {
      earningsThisMonth: 1.2,
      earningsLastMonth: 2.4,
      nextPaymentDate: new Date(Date.now() + 1000 * 60 * 60 * 24 * 3)
    },
    publisherInfo: {
      name: 'brave.com',
      icon: 'https://brave.com/static-assets/images/brave-favicon.png',
      registered: true,
      attentionScore: 0.17,
      autoContributeEnabled: true,
      monthlyContribution: 5,
      supportedWalletProviders: []
    },
    externalWallet: {
      provider: 'uphold',
      username: 'brave123',
      status: 'verified'
    },
    summaryData: {
      grantClaims: 10,
      adEarnings: 10,
      autoContributions: 10,
      oneTimeTips: -2,
      monthlyTips: -19
    }
  })

  return {
    get state () { return stateManager.getState() },

    addListener: stateManager.addListener,

    getString (key: string) {
      return localeStrings[key] || 'MISSING'
    },

    refreshPublisherStatus () {
      console.log('refreshPublisherStatus')
    },

    setIncludeInAutoContribute (enabled: boolean) {
      const { publisherInfo } = stateManager.getState()
      if (publisherInfo) {
        stateManager.update({
          publisherInfo: {
            ...publisherInfo,
            autoContributeEnabled: enabled
          }
        })
      }
    },

    hidePublisherUnverifiedNote () {
      stateManager.update({
        hidePublisherUnverifiedNote: true
      })
    },

    openRewardsSettings () {
      console.log('openRewardsSettings')
    },

    handleMonthlyTipAction (action) {
      switch (action) {
        case 'update': {
          console.log('updateMonthlyTip')
          break
        }
        case 'cancel': {
          const { publisherInfo } = stateManager.getState()
          if (publisherInfo) {
            stateManager.update({
              publisherInfo: {
                ...publisherInfo,
                monthlyContribution: 0
              }
            })
          }
        }
      }
    },

    handleExternalWalletAction (action) {
      console.log('externalWalletAction', action)
    }
  }
}

export function Panel () {
  return (
    <App host={createHost()} />
  )
}
