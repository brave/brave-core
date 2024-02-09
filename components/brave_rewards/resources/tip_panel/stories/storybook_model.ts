/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStateManager } from '../../shared/lib/state_manager'
import { optional } from '../../shared/lib/optional'
import { Model, ModelState, defaultState } from '../lib/model'

export function createModel (): Model {
  const stateManager = createStateManager<ModelState>({
    ...defaultState(),
    loading: false,
    error: null,
    monthlyContributionSet: false,
    creatorBanner: {
      name: 'brave.com',
      provider: '',
      title: 'Brave Software',
      description:
        'Thanks for stopping by. Brave is on a mission to fix the web by ' +
        'giving users a safer, faster and better browsing experience ' +
        'while growing support for content creators through a new ' +
        'attention-based ecosystem of rewards. Join us. It’s time to fix the ' +
        'web together!',
      logo: 'https://rewards.brave.com/LH3yQwkb78iP28pJDSSFPJwU',
      background: '',
      links: {
        twitter: 'https://twitter.com/brave',
        youtube: 'https://www.youtube.com/bravesoftware',
        twitch: 'https://twitch.tv/bravesoftware'
      },
      web3Url: 'https://www.brave.com'
    },
    creatorVerified: true,
    creatorWallets: ['gemini'],
    rewardsUser: {
      balance: optional(8.25),
      walletAuthorized: true,
      walletProvider: 'solana',
      reconnectUrl: ''
    }
  })

  return {
    getState: stateManager.getState,
    addListener: stateManager.addListener,
    async sendContribution (amount: number, monthly: boolean) {
      console.log('sendContribution', amount, monthly)
      await new Promise((resolve) => setTimeout(resolve, 3000))
      console.log('send complete')
      return true
    }
  }
}
