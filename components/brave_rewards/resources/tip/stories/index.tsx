/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { App } from '../components/app'
import { DialogArgs, Host, HostState, MediaMetaData } from '../lib/interfaces'
import { HostContext } from '../lib/host_context'
import { LocaleContext } from '../../shared/lib/locale_context'

import { localeStrings } from './locale_strings'

type MediaType = 'none' | 'twitter' | 'reddit' | 'github'

function getMediaMetaData (type: MediaType): MediaMetaData {
  switch (type) {
    case 'twitter':
      return {
        mediaType: 'twitter',
        publisherName: 'brave',
        postId: '1234',
        postTimestamp: new Date(Date.now() - 6000000).toString(),
        postText: 'It\'s all bravey baby'
      }
    case 'github':
      return {
        mediaType: 'github',
        publisherName: 'brave',
        publisherScreenName: 'brave'
      }
    case 'reddit':
      return {
        mediaType: 'reddit',
        publisherName: 'brave',
        postTimestamp: new Date().toISOString(),
        postText: 'It\'s all bravey baby'
      }
    default:
      return {
        mediaType: 'none'
      }
  }
}

function createDialogArgs (): DialogArgs {
  return {
    publisherKey: 'test-publisher',
    url: '',
    mediaMetaData: getMediaMetaData('none'),
    entryPoint: 'one-time'
  }
}

function createHostState (): HostState {
  return {
    publisherInfo: {
      publisherKey: 'brave.com',
      name: 'brave.com',
      title: 'Brave Software',
      description: 'Thanks for stopping by. Brave is on a mission to fix ' +
        'the web by giving users a safer, faster and better browsing experience ' +
        'while growing support for content creators through a new attention-based ' +
        'ecosystem of rewards. Join us. It’s time to fix the web together!',
      background: '',
      logo: 'https://rewards.brave.com/LH3yQwkb78iP28pJDSSFPJwU',
      amounts: [0.25, 2, 10],
      provider: '',
      links: {
        twitter: 'https://twitter.com/brave',
        youtube: 'https://www.youtube.com/bravesoftware'
      },
      status: 2
    },
    balanceInfo: {
      total: 5,
      wallets: {}
    },
    externalWalletInfo: {
      status: 0,
      type: 'uphold'
    },
    rewardsParameters: {
      tipChoices: [0.25, 2, 10],
      monthlyTipChoices: [0.25, 2, 10],
      rate: 0.333,
      autoContributeChoices: [5, 15, 25, 50]
    },
    hostError: undefined,
    nextReconcileDate: new Date(Date.now() + 15 * 14 * 60 * 60 * 1000),
    adsPerHour: 3,
    autoContributeAmount: 15,
    showOnboarding: false,
    tipProcessed: false,
    currentMonthlyTip: 0
  }
}

function createHost (): Host {
  const hostState = createHostState()
  const dialogArgs = createDialogArgs()

  return {
    get state () {
      return hostState
    },
    getString (key) {
      return localeStrings[key] || 'MISSING'
    },
    getDialogArgs () {
      return dialogArgs
    },
    closeDialog () {
      console.log('closeDialog')
    },
    setAdsPerHour (adsPerHour) {
      console.log('setAdsPerHour', adsPerHour)
    },
    setAutoContributeAmount (amount) {
      console.log('setAutoContributeAmount', amount)
    },
    saveOnboardingResult (result) {
      console.log('saveOnboardingResult', result)
    },
    processTip (amount, kind) {
      console.log('processTip', amount, kind)
    },
    shareTip (target) {
      console.log('shareTip', target)
    },
    addListener (callback) {
      callback(hostState)
      return () => {
        // No-op
      }
    }
  }
}

export default {
  title: 'Rewards'
}

export const TipDialog = () => {
  const host = createHost()
  return (
    <HostContext.Provider value={host}>
      <LocaleContext.Provider value={host}>
        <App />
      </LocaleContext.Provider>
    </HostContext.Provider>
  )
}
