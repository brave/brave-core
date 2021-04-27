// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'

import walletPanelDarkTheme from './theme/wallet-panel-dark'
import walletPanelLightTheme from './theme/wallet-panel-light'
import BraveCoreThemeProvider from '../common/BraveCoreThemeProvider'
import { ConnectWithSite } from './components/extension'
import { StyledExtensionWrapper } from './stories/style'
import store from './store'
import * as WalletPanelActions from './actions/wallet_panel_actions'
import { State, WalletPanelReducerState, WalletAccountType } from './constants/types'

type Props = {
  state: WalletPanelReducerState
  actions: typeof WalletPanelActions
  themeType: chrome.braveTheme.ThemeType
}

export const _ConnectWithSite = (props: Props) => {
  const [selectedAccounts, setSelectedAccounts] = React.useState<WalletAccountType[]>([
    props.state.accounts[0]
  ])
  const [readyToConnect, setReadyToConnect] = React.useState<boolean>(false)
  const selectAccount = (account: WalletAccountType) => {
    const newList = [...selectedAccounts, account]
    setSelectedAccounts(newList)
  }
  const removeAccount = (account: WalletAccountType) => {
    const newList = selectedAccounts.filter(
      (accounts) => accounts.id !== account.id
    )
    setSelectedAccounts(newList)
  }
  const onSubmit = () => {
    props.actions.connectToSite({
      selectedAccounts,
      siteToConnectTo: props.state.connectedSiteOrigin
    })
  }
  const primaryAction = () => {
    if (!readyToConnect) {
      setReadyToConnect(true)
    } else {
      onSubmit()
    }
  }
  const secondaryAction = () => {
    if (readyToConnect) {
      setReadyToConnect(false)
    } else {
      props.actions.cancelConnectToSite()
    }
  }
  return (
    <Provider store={store}>
      <BraveCoreThemeProvider
          initialThemeType={props.themeType}
          dark={walletPanelDarkTheme}
          light={walletPanelLightTheme}
      >
        <StyledExtensionWrapper>
          <ConnectWithSite
            siteURL={props.state.connectedSiteOrigin}
            isReady={readyToConnect}
            accounts={props.state.accounts}
            primaryAction={primaryAction}
            secondaryAction={secondaryAction}
            selectAccount={selectAccount}
            removeAccount={removeAccount}
            selectedAccounts={selectedAccounts}
          />
        </StyledExtensionWrapper>
      </BraveCoreThemeProvider>
    </Provider>
  )
}

function showUI () {
  store.dispatch(WalletPanelActions.visibilityChanged())
}

function visibilityChangedListener () {
  if (document.visibilityState === 'visible') {
    showUI()
  }
}

function initialize () {
  store.dispatch(WalletPanelActions.initialize())
  showUI()
  new Promise(resolve => chrome.braveTheme.getBraveThemeType(resolve))
    .then((themeType: chrome.braveTheme.ThemeType) => {
      render(
          <_ConnectWithSite
            themeType={themeType}
            actions={bindActionCreators(WalletPanelActions, store.dispatch.bind(store))}
            state={((store.getState() as any) as State).walletPanelReducer}
          />, document.getElementById('mountPoint'))
    }).catch(() => {
      console.error('Could not render panel')
    })
}

document.addEventListener('DOMContentLoaded', initialize)
document.addEventListener('visibilitychange', visibilityChangedListener)
