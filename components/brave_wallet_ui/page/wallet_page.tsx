// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { connect, Provider } from 'react-redux'
import { bindActionCreators, Dispatch } from 'redux'
import { initLocale } from 'brave-ui'

import * as WalletPageActions from './actions/wallet_page_actions'
import store from './store'

import 'emptykit.css'
import '../../../ui/webui/resources/fonts/poppins.css'
import '../../../ui/webui/resources/fonts/muli.css'

import { WalletWidgetStandIn } from '../stories/style'
import {
  SideNav,
  WalletPageLayout,
  WalletSubViewLayout,
  CryptoView
} from '../components/desktop'
import LegacyApp from '../components/legacy_app'
import {
  NavTypes,
  NavObjectType,
  State,
  WalletPageReducerState
} from '../constants/types'
import { LinkedAccountsOptions, NavOptions, StaticOptions } from '../options/side-nav-options'
import BuySendSwap from '../components/buy-send-swap'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'

type Props = {
  page: WalletPageReducerState
  actions: typeof WalletPageActions
}

function Page (props: Props) {
  const [view, setView] = React.useState<NavTypes>('crypto')
  const [linkedAccounts] = React.useState<NavObjectType[]>(LinkedAccountsOptions)

  // In the future these will be actual paths
  // for example wallet/rewards
  const navigateTo = (path: NavTypes) => {
    setView(path)
  }

  return (
    <WalletPageLayout>
        <SideNav
          navList={NavOptions}
          staticList={StaticOptions}
          selectedButton={view}
          onSubmit={navigateTo}
          linkedAccountsList={linkedAccounts}
        />
        <WalletSubViewLayout>
          {view === 'crypto' ? (
            <CryptoView />
          ) : (
            <div style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
              <h2>{view} view</h2>
            </div>
          )}

        </WalletSubViewLayout>
        <WalletWidgetStandIn>
          <BuySendSwap />
        </WalletWidgetStandIn>
      </WalletPageLayout>
  )
}

function mapStateToProps (state: State): Partial<Props> {
  return {
    page: state.walletPageReducer
  }
}

function mapDispatchToProps (dispatch: Dispatch): Partial<Props> {
  return {
    actions: bindActionCreators(WalletPageActions, store.dispatch.bind(store))
  }
}

const PageWithState = connect(mapStateToProps, mapDispatchToProps)(Page)

function App () {
  const [initialThemeType, setInitialThemeType] = React.useState<chrome.braveTheme.ThemeType>()
  React.useEffect(() => {
    chrome.braveTheme.getBraveThemeType(setInitialThemeType)
  }, [])
  return (
    <Provider store={store}>
      {initialThemeType &&
      <BraveCoreThemeProvider
        initialThemeType={initialThemeType}
        dark={walletDarkTheme}
        light={walletLightTheme}
      >
        <PageWithState
        />
      </BraveCoreThemeProvider>
      }
    </Provider>
  )
}

function initialize () {
  chrome.braveWallet.isNativeWalletEnabled((enabled: boolean) => {
    if (enabled) {
      store.dispatch(WalletPageActions.initialize())
      render(<App />, document.getElementById('root'))
    } else {
      initializeOldWallet()
    }
  })
}

function initializeOldWallet () {
  chrome.braveWallet.shouldPromptForSetup((prompt: boolean) => {
    if (!prompt) {
      chrome.braveWallet.loadUI(() => {
        window.location.href = 'chrome://wallet'
      })
      return
    }
    renderOldWebUIView()
  })
}

function renderOldWebUIView () {
  new Promise(resolve => chrome.braveTheme.getBraveThemeType(resolve))
  .then((themeType: chrome.braveTheme.ThemeType) => {
    window.i18nTemplate.process(window.document, window.loadTimeData)
    if (window.loadTimeData && window.loadTimeData.data_) {
      initLocale(window.loadTimeData.data_)
    }

    render(
      <BraveCoreThemeProvider
        initialThemeType={themeType}
        dark={DarkTheme}
        light={Theme}
      >
        <LegacyApp />
      </BraveCoreThemeProvider>,
      document.getElementById('root')
    )
  })
  .catch(({ message }) => {
    console.error(`Could not mount brave wallet: ${message}`)
  })
}

document.addEventListener('DOMContentLoaded', initialize)
