/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  ModalActivity,
  ModalBackupRestore,
  WalletWrapper,
  WalletEmpty,
  WalletSummary
} from 'brave-ui/features/rewards'
import { WalletAddIcon, WalletImportIcon } from 'brave-ui/components/icons'
import { AlertWallet } from 'brave-ui/features/rewards/walletWrapper'

// Utils
import { getLocale } from '../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
import WalletOff from '../../../../node_modules/brave-ui/features/rewards/walletOff'
import ModalAddFunds from 'brave-ui/features/rewards/modalAddFunds'

interface State {
  modalBackup: boolean,
  modalBackupActive: 'backup' | 'restore'
  modalActivity: boolean
  modalAddFunds: boolean
  currentActivity?: string
}

interface Props extends Rewards.ComponentProps {
}

class PageWallet extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      modalBackup: false,
      modalBackupActive: 'backup',
      modalActivity: false,
      modalAddFunds: false,
      currentActivity: undefined
    }
  }

  get actions () {
    return this.props.actions
  }

  onModalBackupClose = () => {
    this.actions.onModalBackupClose()
  }

  onModalBackupOpen = () => {
    if (this.props.rewardsData.recoveryKey.length === 0) {
      this.actions.getWalletPassphrase()
    }

    this.actions.onModalBackupOpen()
  }

  onModalBackupTabChange = (tabId: 'backup' | 'restore') => {
    this.setState({
      modalBackupActive: tabId
    })
  }

  onModalBackupOnCopy = () => {
    // TODO NZ implement
    console.log('onModalBackupOnCopy')
  }

  onModalBackupOnPrint = () => {
    // TODO NZ implement
    console.log('onModalBackupOnPrint')
  }

  onModalBackupOnSaveFile = () => {
    // TODO NZ implement
    console.log('onModalBackupOnSaveFile')
  }

  onModalBackupOnRestore = (key: string | MouseEvent) => {
    if (typeof key === 'string' && key.length > 0) {
      key = this.pullRecoveryKeyFromFile(key)
      this.actions.recoverWallet(key)
    }
  }

  pullRecoveryKeyFromFile = (key: string) => {
    let recoveryKey = null
    if (key) {
      let messageLines = key.match(/^.+$/gm)
      if (messageLines) {
        let passphraseLine = '' || messageLines[2]
        if (passphraseLine) {
          const passphrasePattern = new RegExp(['Recovery Key:', '(.+)$'].join(' '))
          recoveryKey = (passphraseLine.match(passphrasePattern) || [])[1]
          return recoveryKey
        }
      }
    }
    return key
  }

  onModalBackupOnImport = () => {
    // TODO NZ implement
    console.log('onModalBackupOnImport')
  }

  getConversion = () => {
    const walletInfo = this.props.rewardsData.walletInfo
    return utils.convertBalance(walletInfo.balance.toString(), walletInfo.rates)
  }

  getGrants = () => {
    const grants = this.props.rewardsData.walletInfo.grants
    if (!grants) {
      return []
    }

    return grants.map((grant: Rewards.Grant) => {
      return {
        tokens: utils.convertProbiToFixed(grant.probi),
        expireDate: new Date(grant.expiryTime * 1000).toLocaleDateString()
      }
    })
  }

  onModalActivityToggle = () => {

    let current = this.state.currentActivity
    if (this.state.modalActivity) {
      current = undefined
    }

    this.setState({
      modalActivity: !this.state.modalActivity,
      currentActivity: current
    })
  }

  onModalAddFundsToggle = () => {
    this.setState({
      modalAddFunds: !this.state.modalAddFunds
    })
  }

  onModalActivityAction (action: string) {
    // TODO NZ implement
    console.log(action)
  }

  onModalActivityRemove () {
    // TODO NZ implement
    console.log('onModalActivityRemove')
  }

  walletAlerts = (): AlertWallet | null => {
    const { balance } = this.props.rewardsData.walletInfo
    const { walletRecoverySuccess, walletServerProblem } = this.props.rewardsData.ui

    if (walletServerProblem) {
      return {
        node: <><b>{getLocale('uhOh')}</b> {getLocale('serverNotResponding')}</>,
        type: 'error'
      }
    }

    if (walletRecoverySuccess) {
      return {
        node: <><b>{getLocale('walletRestored')}</b> {getLocale('walletRecoverySuccess', { balance: balance.toString() })}</>,
        type: 'success',
        onAlertClose: () => {
          this.actions.onClearAlert('walletRecoverySuccess')
        }
      }
    }

    return null
  }

  getWalletSummary = () => {
    const { walletInfo, reports } = this.props.rewardsData
    const { rates } = walletInfo

    let reportProps = {}

    const currentTime = new Date()
    const reportKey = `${currentTime.getFullYear()}_${currentTime.getMonth() + 1}`
    const report: Rewards.Report = reports[reportKey]
    if (report) {
      for (let key in report) {
        const item = report[key]

        if (item.length > 1 && key !== 'total') {
          const tokens = utils.convertProbiToFixed(item)
          reportProps[key] = {
            tokens,
            converted: utils.convertBalance(tokens, rates)
          }
        }
      }
    }

    let props = {
      report: reportProps
    }

    if (reports) {
      props['onActivity'] = this.onModalActivityToggle
    }

    return props
  }

  generateActivityModal = () => {
    const { walletInfo, reports } = this.props.rewardsData
    const { rates } = walletInfo

    let props = {
      contributeRows: [],
      transactionRows: []
    }

    let current = this.state.currentActivity
    if (!current) {
      const currentTime = new Date()
      current = `${currentTime.getFullYear()}_${currentTime.getMonth() + 1}`
    }

    return props
  }

  render () {
    const { connectedWallet, recoveryKey, enabledMain, addresses, walletInfo, ui } = this.props.rewardsData
    const { balance } = walletInfo
    const { walletRecoverySuccess, emptyWallet, modalBackup } = ui
    const addressArray = utils.getAddresses(addresses)

    return (
      <>
        <WalletWrapper
          balance={balance.toFixed(1)}
          converted={utils.formatConverted(this.getConversion())}
          actions={[
            {
              name: getLocale('panelAddFunds'),
              action: this.onModalAddFundsToggle,
              icon: <WalletAddIcon />
            },
            {
              name: getLocale('panelWithdrawFunds'),
              action: () => { console.log('panelWithdrawFunds') },
              icon: <WalletImportIcon />
            }
          ]}
          onSettingsClick={this.onModalBackupOpen}
          onActivityClick={this.onModalActivityToggle}
          showCopy={true}
          showSecActions={true}
          grants={this.getGrants()}
          connectedWallet={connectedWallet}
          alert={this.walletAlerts()}
        >
          {
            enabledMain
            ? emptyWallet
              ? <WalletEmpty />
              : <WalletSummary {...this.getWalletSummary()}/>
            : <WalletOff/>
          }
        </WalletWrapper>
        {
          modalBackup
            ? <ModalBackupRestore
              activeTabId={this.state.modalBackupActive}
              backupKey={recoveryKey}
              onTabChange={this.onModalBackupTabChange}
              onClose={this.onModalBackupClose}
              onCopy={this.onModalBackupOnCopy}
              onPrint={this.onModalBackupOnPrint}
              onSaveFile={this.onModalBackupOnSaveFile}
              onRestore={this.onModalBackupOnRestore}
              error={walletRecoverySuccess === false ? getLocale('walletRecoveryFail') : ''}
            />
            : null
        }
        {
          this.state.modalAddFunds
            ? <ModalAddFunds
              onClose={this.onModalAddFundsToggle}
              addresses={addressArray}
            />
            : null
        }
        {
          // TODO NZ add actual data for the whole section
          this.state.modalActivity
            ? <ModalActivity {...this.generateActivityModal()} />
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
