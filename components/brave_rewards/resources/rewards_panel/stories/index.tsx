/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Host, HostState } from '../lib/interfaces'
import { Notification } from '../../shared/components/notifications'
import { localeStrings } from './locale_strings'
import { createStateManager } from '../../shared/lib/state_manager'

import { LocaleContext } from '../../shared/lib/locale_context'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { NotificationCard } from '../components/notification_card'

import { App } from '../components/app'

export default {
  title: 'Rewards/Panel'
}

const locale = {
  getString (key: string) {
    return localeStrings[key] || 'MISSING'
  }
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
    },
    notifications: [
      {
        type: 'backup-wallet',
        id: '1',
        timeStamp: Date.now() - 100
      },
      {
        type: 'external-wallet-verified',
        id: '2',
        timeStamp: Date.now(),
        provider: 'uphold'
      } as any,
      {
        type: 'external-wallet-linking-failed',
        id: '3',
        timeStamp: Date.now(),
        provider: 'uphold',
        reason: 'mismatched-provider-accounts'
      } as any
    ],
    notificationsLastViewed: Date.now() - 60_000
  })

  return {
    get state () { return stateManager.getState() },

    addListener: stateManager.addListener,

    getString: locale.getString,

    refreshPublisherStatus () {
      console.log('refreshPublisherStatus')
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

    clearNotifications () {
      stateManager.update({
        notifications: []
      })
    },

    setNotificationsViewed () {
      stateManager.update({
        notificationsLastViewed: Date.now()
      })
    }
  }
}

export function MainPanel () {
  const [host] = React.useState(() => createHost())
  return (
    <App host={host} />
  )
}

export function Notification () {
  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <div style={{ width: '375px' }}>
          <NotificationCard
            notification={{
              type: 'backup-wallet',
              id: '123',
              timeStamp: Date.now()
            }}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}
