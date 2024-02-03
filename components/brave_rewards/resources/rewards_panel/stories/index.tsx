/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AdaptiveCaptchaInfo, Host, HostState } from '../lib/interfaces'
import { AdaptiveCaptchaView } from '../components/adaptive_captcha_view'
import { Notification } from '../../shared/components/notifications'
import { localeStrings } from './locale_strings'
import { createStateManager } from '../../shared/lib/state_manager'

import { LocaleContext, createLocaleContextForTesting } from '../../shared/lib/locale_context'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { NotificationCard } from '../components/notification_card'

import { App } from '../components/app'

import grantCaptchaImageURL from './grant_captcha_image.png'
import * as mojom from '../../shared/lib/mojom'
import { optional } from '../../shared/lib/optional'

export default {
  title: 'Rewards/Panel'
}

const locale = createLocaleContextForTesting(localeStrings)

function actionLogger (name: string) {
  return (...args: any[]) => {
    console.log(name, ...args)
  }
}

function createHost (): Host {
  const stateManager = createStateManager<HostState>({
    openTime: Date.now(),
    loading: false,
    requestedView: null,
    rewardsEnabled: true,
    settings: {
      autoContributeEnabled: true,
      autoContributeAmount: 5
    },
    options: {
      autoContributeAmounts: [1, 5, 10, 15],
      externalWalletRegions: new Map([
        ['uphold', { allow: ['US'], block: [] }],
        ['gemini', { allow: [], block: ['US'] }]
      ]),
      vbatDeadline: Date.parse('2023-01-01T00:00:00-05:00'),
      vbatExpired: false
    },
    grantCaptchaInfo: null && {
      id: '123',
      imageURL: grantCaptchaImageURL,
      hint: 'square',
      status: 'pending',
      verifying: false,
      grantInfo: {
        id: 'grant123',
        createdAt: Date.now(),
        claimableUntil: Date.now() + 120_000,
        expiresAt: Date.now() + 120_000,
        amount: 10,
        type: 'ads'
      }
    },
    adaptiveCaptchaInfo: null && {
      url: '',
      status: 'pending'
    },
    externalWalletProviders: ['uphold', 'gemini', 'solana'],
    balance: optional(10.2),
    exchangeInfo: {
      rate: 0.75,
      currency: 'USD'
    },
    earningsInfo: {
      minEarningsThisMonth: 1.2,
      maxEarningsThisMonth: 2.1,
      minEarningsLastMonth: 2.0,
      maxEarningsLastMonth: 2.4,
      nextPaymentDate: Date.now() + 1000 * 60 * 60 * 24 * 3
    },
    payoutStatus: {
      uphold: 'complete'
    },
    publisherInfo: {
      id: 'brave.com',
      name: 'brave.com',
      verified: true,
      icon: 'https://brave.com/static-assets/images/brave-favicon.png',
      platform: null,
      attentionScore: 0.17,
      autoContributeEnabled: true,
      monthlyTip: 5,
      supportedWalletProviders: ['uphold']
    },
    publisherRefreshing: false,
    externalWallet: {
      provider: 'uphold',
      username: 'brave123',
      status: mojom.WalletStatus.kConnected,
      links: {}
    },
    summaryData: {
      adEarnings: 10,
      autoContributions: 10,
      oneTimeTips: -2,
      monthlyTips: -19
    },
    notifications: [
      {
        type: 'monthly-tip-completed',
        id: '1',
        timeStamp: Date.now() - 100
      }
    ] && [],
    availableCountries: ['US'],
    defaultCountry: 'US',
    declaredCountry: 'US',
    userType: 'unconnected',
    publishersVisitedCount: 4,
    selfCustodyInviteDismissed: false
  })

  return {
    get state () { return stateManager.getState() },

    addListener: stateManager.addListener,

    refreshPublisherStatus () {
      console.log('refreshPublisherStatus')
    },

    enableRewards () {
      stateManager.update({
        rewardsEnabled: true,
        declaredCountry: 'US'
      })
      return Promise.resolve('success')
    },

    setIncludeInAutoContribute (enabled) {
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

    openAdaptiveCaptchaSupport () {
      console.log('openAdaptiveCaptchaSupport')
    },

    openRewardsSettings () {
      console.log('openRewardsSettings')
    },

    sendTip () {
      console.log('sendTip')
    },

    handleExternalWalletAction (action) {
      console.log('externalWalletAction', action)
    },

    handleNotificationAction (action) {
      console.log('notificationAction', action)
    },

    dismissNotification (notification) {
      const { notifications } = stateManager.getState()
      stateManager.update({
        notifications: notifications.filter((n) => {
          return n.id !== notification.id
        })
      })
    },

    dismissSelfCustodyInvite () {
      console.log('dismissSelfCustodyInvite')
    },

    solveGrantCaptcha (solution) {
      console.log('solveGrantCaptcha', solution)
      const { grantCaptchaInfo } = stateManager.getState()
      if (!grantCaptchaInfo) {
        return
      }
      stateManager.update({
        grantCaptchaInfo: {
          ...grantCaptchaInfo,
          status: 'passed'
        }
      })
    },

    clearGrantCaptcha () {
      stateManager.update({
        grantCaptchaInfo: null
      })
    },

    clearAdaptiveCaptcha () {
      stateManager.update({
        adaptiveCaptchaInfo: null
      })
    },

    handleAdaptiveCaptchaResult (result) {
      const { adaptiveCaptchaInfo } = stateManager.getState()
      if (!adaptiveCaptchaInfo) {
        return
      }

      console.log('handleAdaptiveCaptcha', result)

      switch (result) {
        case 'success':
          stateManager.update({
            adaptiveCaptchaInfo: { ...adaptiveCaptchaInfo, status: 'success' }
          })
          break
        case 'failure':
        case 'error':
          break
      }
    },

    closePanel() {
      console.log('closePanel')
    },

    onAppRendered () {
      console.log('onAppRendered')
    }
  }
}

export function MainPanel () {
  const [host] = React.useState(() => createHost())
  return (
    <div>
      <LocaleContext.Provider value={locale}>
        <App host={host} />
      </LocaleContext.Provider>
    </div>
  )
}

export function Notification () {
  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <div style={{ width: '375px' }}>
          <NotificationCard
            notification={{
              type: 'monthly-tip-completed',
              id: '123',
              timeStamp: Date.now()
            }}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

export function ExternalWalletDisconnectedNotification () {
  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <div style={{ width: '375px' }}>
          <NotificationCard
            notification={{
              type: 'external-wallet-disconnected',
              id: '123',
              timeStamp: Date.now(),
              provider: 'Uphold'
            } as any}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

export function GrantNotification () {
  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <div style={{ width: '375px' }}>
          <NotificationCard
            notification={{
              type: 'grant-available',
              id: '123',
              grantInfo: {
                id: '123',
                type: 'ads',
                amount: 1.25,
                createdAt: Date.now(),
                expiresAt: Date.now() + 1000 * 60 * 60 * 24 * 5
              },
              timeStamp: Date.now()
            } as any}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

export function AdaptiveCaptcha () {
  const adaptiveCaptchaInfo: AdaptiveCaptchaInfo = {
    url: '',
    status: 'pending'
  }
  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <div>
          <AdaptiveCaptchaView
            adaptiveCaptchaInfo={adaptiveCaptchaInfo}
            onClose={actionLogger('onClose')}
            onCaptchaResult={actionLogger('onCaptchaResult')}
            onContactSupport={actionLogger('onContactSupport')}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}
