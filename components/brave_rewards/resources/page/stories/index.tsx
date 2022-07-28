/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createStore } from 'redux'
import { Provider } from 'react-redux'
import * as knobs from '@storybook/addon-knobs'

import { ConnectWalletModal } from '../components/connect_wallet_modal'
import { ClaimGrantView } from '../components/claim_grant_view'
import { Settings } from '../components/settings'
import { LocaleContext } from '../../shared/lib/locale_context'
import { LayoutManager } from '../components/layout_manager'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { localeStrings } from './locale_strings'
import { reduxState } from './redux_state'

export default {
  title: 'Rewards/Settings'
}

const locale = {
  getString (key: string) {
    return localeStrings[key] || 'MISSING'
  }
}

function actionLogger (name: string) {
  return (...args: any[]) => {
    console.log(name, ...args)
  }
}

export function ConnectWallet () {
  const providers = [
    {
      type: 'uphold',
      name: 'Uphold'
    },
    {
      type: 'gemini',
      name: 'Gemini'
    }
  ]

  const narrow = knobs.boolean('Narrow', false)

  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <LayoutManager layout={narrow ? 'narrow' : 'wide'}>
          <ConnectWalletModal
            rewardsBalance={1}
            providers={providers}
            onContinue={actionLogger('onContinue')}
            onClose={actionLogger('onClose')}
          />
        </LayoutManager>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

export function Claim () {
  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <div style={{ width: '366px' }}>
          <ClaimGrantView
            grantInfo={{
              id: 'grant-1',
              type: 'ads',
              amount: 3.25,
              createdAt: Date.now(),
              claimableUntil: Date.now() + 1000 * 60 * 60 * 24 * 5,
              expiresAt: Date.now() + 1000 * 60 * 60 * 24 * 5
            }}
            onClaim={actionLogger('onClaim')}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

export function Page () {
  const store = createStore((state) => state, reduxState)
  const narrow = knobs.boolean('Narrow', false)
  return (
    <Provider store={store}>
      <LocaleContext.Provider value={locale}>
        <WithThemeVariables>
          <LayoutManager layout={narrow ? 'narrow' : 'wide'}>
            <Settings />
          </LayoutManager>
        </WithThemeVariables>
      </LocaleContext.Provider>
    </Provider>
  )
}
