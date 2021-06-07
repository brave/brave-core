// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { connect } from 'react-redux'
import { bindActionCreators, Dispatch } from 'redux'

import * as WalletPageActions from './actions/wallet_page_actions'
import * as WalletActions from '../common/actions/wallet_actions'
import store from './store'

import 'emptykit.css'
import '../../../ui/webui/resources/fonts/poppins.css'
import '../../../ui/webui/resources/fonts/muli.css'

import { WalletWidgetStandIn } from '../stories/style'
import {
  SideNav,
  WalletPageLayout,
  WalletSubViewLayout,
  CryptoView,
  LockScreen
} from '../components/desktop'
import {
  NavTypes,
  WalletState,
  PageState,
  WalletPageState
} from '../constants/types'
import { NavOptions } from '../options/side-nav-options'
import BuySendSwap from '../components/buy-send-swap'
import Onboarding from '../stories/screens/onboarding'
import BackupWallet from '../stories/screens/backup-wallet'

type Props = {
  wallet: WalletState
  page: PageState
  walletPageActions: typeof WalletPageActions
  walletActions: typeof WalletActions
}

function Container (props: Props) {
  const [view, setView] = React.useState<NavTypes>('crypto')
  const [inputValue, setInputValue] = React.useState<string>('')
  const [hasPasswordError, setHasPasswordError] = React.useState<boolean>(false)

  // In the future these will be actual paths
  // for example wallet/rewards
  const navigateTo = (path: NavTypes) => {
    setView(path)
  }

  // recoveryVerified Prop will be used in a future PR.
  const completeWalletSetup = (recoveryVerified: boolean) => {
    if (recoveryVerified) {
      props.walletPageActions.walletBackupComplete()
    }
    props.walletPageActions.walletSetupComplete()
  }

  const onBackupWallet = () => {
    props.walletPageActions.walletBackupComplete()
  }

  // Need to wire up restore wallet
  const restoreWallet = (phrase: string, password: string) => {
    // Logic here to restore wallet and set password
  }

  const passwordProvided = (password: string) => {
    props.walletPageActions.createWallet({ password })
  }

  // Need to wire up incorrect password logic
  const unlockWallet = () => {
    // Logic here to setHassPasswordError if password was incorrect
    props.walletActions.unlockWallet({ password: inputValue })
  }

  const lockWallet = () => {
    props.walletActions.lockWallet()
  }

  const onShowBackup = () => {
    props.walletPageActions.showRecoveryPhrase(true)
  }

  const onHideBackup = () => {
    props.walletPageActions.showRecoveryPhrase(false)
  }

  const handlePasswordChanged = (value: string) => {
    setInputValue(value)
    setHasPasswordError(false)
  }

  const recoveryPhrase = (props.page.mnemonic || '').split(' ')
  if (!props.wallet.isWalletCreated) {
    return (
      <WalletPageLayout>
        <Onboarding
          recoveryPhrase={recoveryPhrase}
          onPasswordProvided={passwordProvided}
          onSubmit={completeWalletSetup}
          onRestore={restoreWallet}
        />
      </WalletPageLayout>
    )
  }

  return (
    <WalletPageLayout>
      <SideNav
        navList={NavOptions}
        selectedButton={view}
        onSubmit={navigateTo}
      />
      <WalletSubViewLayout>
        {view === 'crypto' ? (
          <>
            {props.wallet.isWalletLocked ? (
              <LockScreen
                onSubmit={unlockWallet}
                disabled={inputValue === ''}
                onPasswordChanged={handlePasswordChanged}
                hasPasswordError={hasPasswordError}
              />
            ) : (
              <>
                {props.page.showRecoveryPhrase ? (
                  <BackupWallet
                    isOnboarding={false}
                    onCancel={onHideBackup}
                    onSubmit={onBackupWallet}
                    recoveryPhrase={recoveryPhrase}
                  />
                ) : (
                  <CryptoView
                    onLockWallet={lockWallet}
                    needsBackup={!props.wallet.isWalletBackedUp}
                    onShowBackup={onShowBackup}
                  />
                )}
              </>
            )}
          </>
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

function mapStateToProps (state: WalletPageState): Partial<Props> {
  return {
    page: state.page,
    wallet: state.wallet
  }
}

function mapDispatchToProps (dispatch: Dispatch): Partial<Props> {
  return {
    walletPageActions: bindActionCreators(WalletPageActions, store.dispatch.bind(store)),
    walletActions: bindActionCreators(WalletActions, store.dispatch.bind(store))
  }
}

export default connect(mapStateToProps, mapDispatchToProps)(Container)
