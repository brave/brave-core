// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { connect, Provider } from 'react-redux'
import { bindActionCreators, Dispatch } from 'redux'

import walletPanelDarkTheme from './theme/wallet-panel-dark'
import walletPanelLightTheme from './theme/wallet-panel-light'
import BraveCoreThemeProvider from '../common/BraveCoreThemeProvider'
import { ConnectWithSite } from './components/extension'
import { StyledExtensionWrapper } from './stories/style'
import store from './store'
import * as WalletPanelActions from './actions/wallet_panel_actions'
import { State, WalletPanelReducerState, WalletAccountType } from './constants/types'

type Props = {
  panel: WalletPanelReducerState
  actions: typeof WalletPanelActions
}

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
        dark={walletPanelDarkTheme}
        light={walletPanelLightTheme}
      >
        <PanelWithState
        />
      </BraveCoreThemeProvider>
      }
    </Provider>
  )
}

function Panel (props: Props) {
  const [selectedAccounts, setSelectedAccounts] = React.useState<WalletAccountType[]>([
    props.panel.accounts[0]
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
      siteToConnectTo: props.panel.connectedSiteOrigin
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
    <StyledExtensionWrapper>
      <ConnectWithSite
        siteURL={props.panel.connectedSiteOrigin}
        isReady={readyToConnect}
        accounts={props.panel.accounts}
        primaryAction={primaryAction}
        secondaryAction={secondaryAction}
        selectAccount={selectAccount}
        removeAccount={removeAccount}
        selectedAccounts={selectedAccounts}
      />
    </StyledExtensionWrapper>
  )
}

function mapStateToProps (state: State): Partial<Props> {
  return {
    panel: state.walletPanelReducer
  }
}

function mapDispatchToProps (dispatch: Dispatch): Partial<Props> {
  return {
    actions: bindActionCreators(WalletPanelActions, store.dispatch.bind(store))
  }
}

const PanelWithState = connect(mapStateToProps, mapDispatchToProps)(Panel)

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
  render(<App />, document.getElementById('mountPoint'))
}

document.addEventListener('DOMContentLoaded', initialize)
document.addEventListener('visibilitychange', visibilityChangedListener)
