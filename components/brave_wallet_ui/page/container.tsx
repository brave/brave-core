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
  WalletPageState,
  ChartTimelineType,
  AssetOptionType
} from '../constants/types'
import { NavOptions } from '../options/side-nav-options'
import BuySendSwap from '../components/buy-send-swap'
import Onboarding from '../stories/screens/onboarding'
import BackupWallet from '../stories/screens/backup-wallet'
import { formatePrices } from '../utils/format-prices'

type Props = {
  wallet: WalletState
  page: PageState
  walletPageActions: typeof WalletPageActions
  walletActions: typeof WalletActions
}

function Container (props: Props) {
  // Wallet Props
  const {
    isWalletCreated,
    isWalletLocked,
    isWalletBackedUp,
    hasIncorrectPassword,
    accounts
  } = props.wallet

  // Page Props
  const {
    showRecoveryPhrase,
    invalidMnemonic,
    mnemonic,
    selectedTimeline,
    selectedAsset,
    selectedAssetPrice,
    selectedAssetPriceHistory
  } = props.page

  const [view, setView] = React.useState<NavTypes>('crypto')
  const [inputValue, setInputValue] = React.useState<string>('')

  // In the future these will be actual paths
  // for example wallet/rewards
  const navigateTo = (path: NavTypes) => {
    setView(path)
  }

  const completeWalletSetup = (recoveryVerified: boolean) => {
    if (recoveryVerified) {
      props.walletPageActions.walletBackupComplete()
    }
    props.walletPageActions.walletSetupComplete()
  }

  const onBackupWallet = () => {
    props.walletPageActions.walletBackupComplete()
  }

  const restoreWallet = (mnemonic: string, password: string) => {
    props.walletPageActions.restoreWallet({ mnemonic, password })
  }

  const passwordProvided = (password: string) => {
    props.walletPageActions.createWallet({ password })
  }

  const unlockWallet = () => {
    props.walletActions.unlockWallet({ password: inputValue })
    setInputValue('')
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
    if (hasIncorrectPassword) {
      props.walletActions.hasIncorrectPassword(false)
    }
  }

  const restorError = React.useMemo(() => {
    if (invalidMnemonic) {
      setTimeout(function () { props.walletPageActions.hasMnemonicError(false) }, 5000)
      return true
    }
    return false
  }, [invalidMnemonic])

  const recoveryPhrase = (mnemonic || '').split(' ')

  // Will need to add additional logic here and in the reducer
  // to fetch price history once the pricing api is read.
  const onChangeTimeline = (timeline: ChartTimelineType) => {
    props.walletPageActions.changeTimline(timeline)
  }

  // Will need to add additional logic here to multiply
  // the fullbalance by the current market price of "ETH" to start.
  const portfolioBalance = React.useMemo(() => {
    const balances = accounts.map((account) => {
      return account.balance
    })
    const fullBalance = balances.reduce(function (a, b) {
      return a + b
    }, 0)
    const grandTotal = fullBalance * 1
    return formatePrices(grandTotal)
  }, [accounts])

  const onSelectAsset = (asset: AssetOptionType) => {
    props.walletPageActions.selectAsset(asset)
  }

  const renderWallet = React.useMemo(() => {
    if (!isWalletCreated) {
      return (
        <Onboarding
          recoveryPhrase={recoveryPhrase}
          onPasswordProvided={passwordProvided}
          onSubmit={completeWalletSetup}
          onRestore={restoreWallet}
          hasRestoreError={restorError}
        />
      )
    } else {
      return (
        <>
          {isWalletLocked ? (
            <LockScreen
              onSubmit={unlockWallet}
              disabled={inputValue === ''}
              onPasswordChanged={handlePasswordChanged}
              hasPasswordError={hasIncorrectPassword}
            />
          ) : (
            <>
              {showRecoveryPhrase ? (
                <BackupWallet
                  isOnboarding={false}
                  onCancel={onHideBackup}
                  onSubmit={onBackupWallet}
                  recoveryPhrase={recoveryPhrase}
                />
              ) : (
                <CryptoView
                  onLockWallet={lockWallet}
                  needsBackup={!isWalletBackedUp}
                  onShowBackup={onShowBackup}
                  accounts={accounts}
                  onChangeTimeline={onChangeTimeline}
                  onSelectAsset={onSelectAsset}
                  portfolioBalance={portfolioBalance}
                  selectedAsset={selectedAsset}
                  selectedAssetPrice={selectedAssetPrice}
                  selectedAssetPriceHistory={selectedAssetPriceHistory}
                  selectedTimeline={selectedTimeline}
                  transactions={[]}
                  userAssetList={[]}
                />
              )}
            </>
          )}
        </>
      )
    }
  }, [
    isWalletCreated,
    isWalletLocked,
    recoveryPhrase,
    isWalletBackedUp,
    inputValue,
    hasIncorrectPassword,
    showRecoveryPhrase
  ])

  return (
    <WalletPageLayout>
      <SideNav
        navList={NavOptions}
        selectedButton={view}
        onSubmit={navigateTo}
      />
      <WalletSubViewLayout>
        {view === 'crypto' ? (
          renderWallet
        ) : (
          <div style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
            <h2>{view} view</h2>
          </div>
        )}
      </WalletSubViewLayout>
      <WalletWidgetStandIn>
        {isWalletCreated && !isWalletLocked &&
          <BuySendSwap />
        }
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
