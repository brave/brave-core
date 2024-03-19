/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createStore } from 'redux'
import { Provider } from 'react-redux'
import * as knobs from '@storybook/addon-knobs'

import { ExternalWalletProvider } from '../../shared/lib/external_wallet'
import { ConnectWalletModal } from '../components/connect_wallet_modal'
import { Settings } from '../components/settings'
import { PlatformContext } from '../lib/platform_context'
import { LocaleContext, createLocaleContextForTesting } from '../../shared/lib/locale_context'
import { LayoutManager } from '../components/layout_manager'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { localeStrings } from './locale_strings'
import { reduxState } from './redux_state'

export default {
  title: 'Rewards/Settings'
}

const locale = createLocaleContextForTesting(localeStrings)

function actionLogger (name: string) {
  return (...args: any[]) => {
    console.log(name, ...args)
  }
}

export function ConnectWallet () {
  const providers: Array<{ provider: ExternalWalletProvider, enabled: boolean }> = [
    {
      provider: 'uphold',
      enabled: true
    },
    {
      provider: 'gemini',
      enabled: false
    },
    {
      provider: 'solana',
      enabled: true
    }
  ]

  const narrow = knobs.boolean('Narrow', false)

  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <LayoutManager layout={narrow ? 'narrow' : 'wide'}>
          <ConnectWalletModal
            currentCountryCode='US'
            providers={providers}
            connectState='error'
            onContinue={actionLogger('onContinue')}
            onClose={actionLogger('onClose')}
          />
        </LayoutManager>
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
          <PlatformContext.Provider value={{ isAndroid: false }}>
            <div style={{ width: narrow ? '375px' : 'auto' }}>
              <LayoutManager layout={narrow ? 'narrow' : 'wide'}>
                <Settings />
              </LayoutManager>
            </div>
          </PlatformContext.Provider>
        </WithThemeVariables>
      </LocaleContext.Provider>
    </Provider>
  )
}
