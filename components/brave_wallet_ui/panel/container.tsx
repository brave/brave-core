// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { connect } from 'react-redux'
import { bindActionCreators, Dispatch } from 'redux'
import { ConnectWithSite, WelcomePanel } from '../components/extension'
import { StyledExtensionWrapper } from '../stories/style'
import store from './store'
import * as WalletPanelActions from './actions/wallet_panel_actions'
import { WalletState, PanelState, WalletPanelState, WalletAccountType } from '../constants/types'
import LockPanel from '../components/extension/lock-panel'

type Props = {
  panel: PanelState
  wallet: WalletState
  actions: typeof WalletPanelActions
}

function mapStateToProps (state: WalletPanelState): Partial<Props> {
  return {
    panel: state.panel,
    wallet: state.wallet
  }
}

function mapDispatchToProps (dispatch: Dispatch): Partial<Props> {
  return {
    actions: bindActionCreators(WalletPanelActions, store.dispatch.bind(store))
  }
}

function Panel (props: Props) {
  // TODO(petemill): If initial data or UI takes a noticeable amount of time to arrive
  // consider rendering a "loading" indicator when `hasInitialized === false`, and
  // also using `React.lazy` to put all the main UI in a separate JS bundle and display
  // that loading indicator ASAP.
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
  const [inputValue, setInputValue] = React.useState<string>('')
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
  const unlockWallet = () => {
  }
  const handlePasswordChanged = (value: string) => {
    setInputValue(value)
  }
  const onRestore = () => {
    chrome.tabs.create({ url: 'chrome://wallet#restore' })
  }
  const onSetup = () => {
    chrome.tabs.create({ url: 'chrome://wallet' })
  }

  return (
    <StyledExtensionWrapper>
      { !props.wallet.isWalletCreated ?
        (<WelcomePanel onRestore={onRestore} onSetup={onSetup} />)
        : props.wallet.isWalletLocked ?
        (<LockPanel onSubmit={unlockWallet} disabled={inputValue === ''} onPasswordChanged={handlePasswordChanged} />)
        :
        (<ConnectWithSite
            siteURL={props.panel.connectedSiteOrigin}
            isReady={readyToConnect}
            accounts={props.panel.accounts}
            primaryAction={primaryAction}
            secondaryAction={secondaryAction}
            selectAccount={selectAccount}
            removeAccount={removeAccount}
            selectedAccounts={selectedAccounts}
        />
        )
      }
    </StyledExtensionWrapper>
  )
}

export default connect(mapStateToProps, mapDispatchToProps)(Panel)
