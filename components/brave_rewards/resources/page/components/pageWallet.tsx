/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

import { WalletCard, ExternalWalletAction } from '../../shared/components/wallet_card'

import {
  ExternalWallet,
  ExternalWalletProvider,
  externalWalletProviderFromString,
  isExternalWalletProviderAllowed
} from '../../shared/lib/external_wallet'

import * as rewardsActions from '../actions/rewards_actions'
import { ConnectWalletModal } from './connect_wallet_modal'
import { TosUpdateModal } from './tos_update_modal'
import { ResetModal } from './reset_modal'

import * as mojom from '../../shared/lib/mojom'

import * as Rewards from '../lib/types'

interface Props {
  rewardsData: Rewards.State
  actions: any
}

class PageWallet extends React.Component<Props> {
  constructor (props: Props) {
    super(props)
  }

  get actions () {
    return this.props.actions
  }

  componentDidMount () {
    this.isBackupUrl()
    this.isVerifyUrl()
    this.actions.getExternalWalletProviders()
  }

  onModalResetClose = () => {
    // Used by the settings page to clear browsing data.
    if (this.urlHashIs('#reset')) {
      window.location.hash = ''
    }
    this.actions.onModalResetClose()
  }

  onModalResetOpen = () => {
    this.actions.onModalResetOpen()
  }

  onModalResetOnReset = () => {
    this.actions.onModalResetClose()
    this.actions.completeReset()
  }

  urlHashIs = (hash: string) => {
    return (
      window &&
      window.location &&
      window.location.hash &&
      window.location.hash === hash
    )
  }

  isBackupUrl = () => {
    if (this.urlHashIs('#reset')) {
      this.onModalResetOpen()
    }
  }

  isVerifyUrl = () => {
    if (this.urlHashIs('#verify')) {
      this.handleExternalWalletLink()
    }
  }

  toggleVerifyModal = () => {
    if (this.props.rewardsData.ui.modalConnect) {
      window.history.replaceState({}, 'Rewards', '/')
      this.actions.onModalConnectClose()
    } else {
      this.actions.onModalConnectOpen()
    }
  }

  handleExternalWalletLink = () => {
    const { externalWallet } = this.props.rewardsData
    if (externalWallet) {
      switch (externalWallet.status) {
        case mojom.WalletStatus.kConnected:
          return
        case mojom.WalletStatus.kLoggedOut:
          this.actions.beginExternalWalletLogin(externalWallet.type)
          return
      }
    }
    this.toggleVerifyModal()
  }

  onConnectWalletContinue = (provider: string) => {
    this.actions.beginExternalWalletLogin(provider)
  }

  getExternalWalletStatus = (): mojom.WalletStatus | null => {
    const { externalWallet } = this.props.rewardsData
    if (!externalWallet) {
      return null
    }

    return externalWallet.status
  }

  getExternalWalletProvider = (): ExternalWalletProvider | null => {
    const { externalWallet } = this.props.rewardsData
    if (!externalWallet) {
      return null
    }
    return externalWalletProviderFromString(externalWallet.type)
  }

  goToExternalWallet = () => {
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet || !externalWallet.accountUrl) {
      this.actions.getExternalWallet()
      return
    }

    window.open(externalWallet.accountUrl, '_blank', 'noreferrer')
  }

  isWalletProviderEnabled = (walletProvider: string) => {
    const { currentCountryCode, parameters } = this.props.rewardsData
    const regions = parameters.walletProviderRegions[walletProvider] || null
    return isExternalWalletProviderAllowed(currentCountryCode, regions)
  }

  generateExternalWalletProviderList = (walletProviders: string[]) => {
    const list: Array<{ provider: ExternalWalletProvider, enabled: boolean}> = []
    for (const type of walletProviders) {
      const provider = externalWalletProviderFromString(type)
      if (provider) {
        list.push({
          provider,
          enabled: this.isWalletProviderEnabled(type)
        })
      }
    }
    return list
  }

  onExternalWalletAction = (action: ExternalWalletAction) => {
    switch (action) {
      case 'reconnect':
      case 'verify':
        this.handleExternalWalletLink()
        break
      case 'view-account':
        this.goToExternalWallet()
        break
    }
  }

  render () {
    const {
      adsData,
      balance,
      balanceReport,
      enabledContribute,
      externalWalletProviderList,
      ui,
      externalWallet,
      parameters,
      userType,
      isUserTermsOfServiceUpdateRequired
    } = this.props.rewardsData
    const { modalReset, modalConnect } = ui

    const walletStatus = this.getExternalWalletStatus()
    const walletProvider = this.getExternalWalletProvider()

    let externalWalletInfo: ExternalWallet | null = null

    if (externalWallet && walletStatus && walletProvider) {
      externalWalletInfo = {
        provider: walletProvider,
        authenticated: walletStatus === mojom.WalletStatus.kConnected,
        name: externalWallet.userName || '',
        url: ''
      }
    }

    const summaryData = {
      adEarnings: balanceReport && balanceReport.ads || 0,
      autoContributions: balanceReport && balanceReport.contribute || 0,
      oneTimeTips: balanceReport && balanceReport.tips || 0,
      monthlyTips: balanceReport && balanceReport.monthly || 0
    }

    return (
      <>
        {
          userType !== 'unconnected' &&
            <WalletCard
              userType={userType}
              balance={balance}
              externalWallet={externalWalletInfo}
              providerPayoutStatus={'off'}
              minEarningsThisMonth={adsData.adsMinEarningsThisMonth || 0}
              maxEarningsThisMonth={adsData.adsMaxEarningsThisMonth || 0}
              minEarningsLastMonth={adsData.adsMinEarningsLastMonth || 0}
              maxEarningsLastMonth={adsData.adsMaxEarningsLastMonth || 0}
              nextPaymentDate={0}
              exchangeRate={parameters.rate}
              exchangeCurrency={'USD'}
              showSummary={true}
              summaryData={summaryData}
              autoContributeEnabled={enabledContribute}
              onExternalWalletAction={this.onExternalWalletAction}
            />
        }
        {
          modalReset ?
            <ResetModal
              onClose={this.onModalResetClose}
              onReset={this.onModalResetOnReset}
            /> :
          isUserTermsOfServiceUpdateRequired ?
            <TosUpdateModal /> : null
        }
        {
          modalConnect
            ? <ConnectWalletModal
                currentCountryCode={this.props.rewardsData.currentCountryCode}
                providers={this.generateExternalWalletProviderList(externalWalletProviderList)}
                connectState={this.props.rewardsData.ui.modalConnectState}
                onContinue={this.onConnectWalletContinue}
                onClose={this.toggleVerifyModal}
            />
            : null
        }
      </>
    )
  }
}

const mapStateToProps = (state: Rewards.ApplicationState) => ({
  rewardsData: state.rewardsData
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(PageWallet)
