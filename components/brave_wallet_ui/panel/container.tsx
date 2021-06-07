// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { connect } from 'react-redux'
import { bindActionCreators, Dispatch } from 'redux'
import { ConnectWithSite, ConnectedPanel, Panel, WelcomePanel } from '../components/extension'
import { AppList } from '../components/shared'
import { filterAppList } from '../utils/filter-app-list'
import { ScrollContainer, StyledExtensionWrapper } from '../stories/style'
import store from './store'
import * as WalletPanelActions from './actions/wallet_panel_actions'
import * as WalletActions from '../common/actions/wallet_actions'
import { AppObjectType, AppsListType, WalletState, PanelState, PanelTypes, WalletPanelState, WalletAccountType } from '../constants/types'
import { AppsList } from '../options/apps-list-options'
import LockPanel from '../components/extension/lock-panel'

type Props = {
  panel: PanelState
  wallet: WalletState
  walletPanelActions: typeof WalletPanelActions
  walletActions: typeof WalletActions
}

function mapStateToProps (state: WalletPanelState): Partial<Props> {
  return {
    panel: state.panel,
    wallet: state.wallet
  }
}

function mapDispatchToProps (dispatch: Dispatch): Partial<Props> {
  return {
    walletPanelActions: bindActionCreators(WalletPanelActions, store.dispatch.bind(store)),
    walletActions: bindActionCreators(WalletActions, store.dispatch.bind(store))
  }
}

function Container (props: Props) {
  // TODO(petemill): If initial data or UI takes a noticeable amount of time to arrive
  // consider rendering a "loading" indicator when `hasInitialized === false`, and
  // also using `React.lazy` to put all the main UI in a separate JS bundle and display
  // that loading indicator ASAP.
  const [selectedAccounts, setSelectedAccounts] = React.useState<WalletAccountType[]>([])
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)
  const [walletConnected, setWalletConnected] = React.useState<boolean>(true)
  const [hasPasswordError, setHasPasswordError] = React.useState<boolean>(false)
  const toggleConnected = () => {
    setWalletConnected(!walletConnected)
  }

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
    props.walletPanelActions.connectToSite({
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
      props.walletPanelActions.cancelConnectToSite()
    }
  }
  // Need to wire up incorrect password logic
  const unlockWallet = () => {
    // Logic here to setHassPasswordError if password was incorrect
    props.walletActions.unlockWallet({ password: inputValue })
  }
  const handlePasswordChanged = (value: string) => {
    setHasPasswordError(false)
    setInputValue(value)
  }
  const onRestore = () => {
    props.walletPanelActions.restoreWallet()
  }
  const onSetup = () => {
    props.walletPanelActions.setupWallet()
  }
  const addToFavorites = (app: AppObjectType) => {
    props.walletActions.addFavoriteApp(app)
  }

  const navigateTo = (selectedPanel: PanelTypes) => {
    if (selectedPanel === 'expanded') {
      props.walletPanelActions.expandWallet()
    } else {
      props.walletPanelActions.navigateTo(selectedPanel)
    }
  }

  const browseMore = () => {
    props.walletPanelActions.openWalletApps()
  }

  const removeFromFavorites = (app: AppObjectType) => {
    props.walletActions.removeFavoriteApp(app)
  }

  const filterList = (event: any) => {
    filterAppList(event, AppsList, setFilteredAppsList)
  }

  if (!props.wallet.hasInitialized || !props.wallet.accounts) {
    return null
  }

  if (!props.wallet.isWalletCreated) {
    return (
      <StyledExtensionWrapper>
        <WelcomePanel onRestore={onRestore} onSetup={onSetup} />
      </StyledExtensionWrapper>)
  }

  if (props.wallet.isWalletLocked) {
    return (
      <StyledExtensionWrapper>
        <LockPanel
          hasPasswordError={hasPasswordError}
          onSubmit={unlockWallet}
          disabled={inputValue === ''}
          onPasswordChanged={handlePasswordChanged}
        />
      </StyledExtensionWrapper>)
  }

  if (props.panel.selectedPanel === 'apps') {
    return (
      <StyledExtensionWrapper>
        <Panel
          navAction={navigateTo}
          title={props.panel.panelTitle}
          useSearch={props.panel.selectedPanel === 'apps'}
          searchAction={props.panel.selectedPanel === 'apps' ? filterList : undefined}
        >
          <ScrollContainer>
            <AppList
              list={filteredAppsList}
              favApps={props.wallet.favoriteApps}
              addToFav={addToFavorites}
              removeFromFav={removeFromFavorites}
              action={browseMore}
            />
          </ScrollContainer>
        </Panel>
      </StyledExtensionWrapper>)
  }

  if (props.panel.selectedPanel === 'connectWithSite') {
    return (
      <StyledExtensionWrapper>
        <ConnectWithSite
          siteURL={props.panel.connectedSiteOrigin}
          isReady={readyToConnect}
          accounts={props.wallet.accounts}
          primaryAction={primaryAction}
          secondaryAction={secondaryAction}
          selectAccount={selectAccount}
          removeAccount={removeAccount}
          selectedAccounts={selectedAccounts}
        />
      </StyledExtensionWrapper>)
  }

  return (
    <>
      <ConnectedPanel
        selectedAccount={props.wallet.accounts[0]}
        isConnected={walletConnected}
        connectAction={toggleConnected}
        navAction={navigateTo}
      />
    </>)

}

export default connect(mapStateToProps, mapDispatchToProps)(Container)
