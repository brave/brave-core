/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'

import { App } from '../components/app'
import { DialogArgs, Host, HostState, MediaMetaData } from '../lib/interfaces'
import { HostContext } from '../lib/host_context'

import { localeStrings } from './locale_strings'

type MediaType = 'none' | 'twitter' | 'reddit' | 'github'

function getMediaMetaData (type: MediaType): MediaMetaData {
  switch (type) {
    case 'twitter':
      return {
        mediaType: 'twitter',
        twitterName: 'brave',
        screenName: 'brave',
        userId: 'brave',
        tweetId: '1234',
        tweetTimestamp: Date.now() / 1000,
        tweetText: 'It\'s all bravey baby'
      }
    case 'github':
      return {
        mediaType: 'github',
        userName: 'brave'
      }
    case 'reddit':
      return {
        mediaType: 'reddit',
        userName: 'brave',
        postRelDate: new Date().toISOString(),
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
      amounts: [1, 10, 100],
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
      token: '',
      address: '1234',
      status: 0,
      type: 'uphold',
      verifyUrl: 'about:blank',
      addUrl: 'about:blank',
      withdrawUrl: 'about:blank',
      userName: 'username',
      accountUrl: 'about:blank',
      loginUrl: 'about:blank'
    },
    rewardsParameters: {
      tipChoices: [1, 2, 3],
      monthlyTipChoices: [1, 2, 3],
      rate: 0.333
    },
    hostError: undefined,
    nextReconcileDate: new Date(Date.now() + 15 * 14 * 60 * 60 * 1000),
    onlyAnonWallet: false,
    tipProcessed: false,
    currentMontlyTip: 0
  }
}

function createHost (): Host {
  const hostState = createHostState()
  const dialogArgs = createDialogArgs()

  return {
    getString (key) {
      return localeStrings[key] || 'MISSING'
    },
    getDialogArgs () {
      return dialogArgs
    },
    closeDialog () {
      console.log('closeDialog')
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

storiesOf('Rewards/Tip', module)
  .add('Tip Dialog', () => {
    return (
      <HostContext.Provider value={createHost()}>
        <App />
      </HostContext.Provider>
    )
  })
