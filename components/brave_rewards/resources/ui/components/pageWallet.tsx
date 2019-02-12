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
import { WalletAddIcon } from 'brave-ui/components/icons'
import { AlertWallet } from 'brave-ui/features/rewards/walletWrapper'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
import WalletOff from 'brave-ui/features/rewards/walletOff'
import ModalAddFunds from 'brave-ui/features/rewards/modalAddFunds'

const clipboardCopy = require('clipboard-copy')

interface State {
  activeTabId: number
  modalBackup: boolean
  modalActivity: boolean
  modalAddFunds: boolean
}

interface Props extends Rewards.ComponentProps {
}

class PageWallet extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      activeTabId: 0,
      modalBackup: false,
      modalActivity: false,
      modalAddFunds: false
    }
  }

  get actions () {
    return this.props.actions
  }

  componentDidMount () {
    this.isAddFundsUrl()
    this.isBackupUrl()
  }

  onModalBackupClose = () => {
    if (this.urlHashIs('#backup-restore')) {
      window.location.hash = ''
    }
    this.actions.onModalBackupClose()
  }

  onModalBackupOpen = () => {
    if (this.props.rewardsData.recoveryKey.length === 0) {
      this.actions.getWalletPassphrase()
    }

    this.actions.onModalBackupOpen()
  }

  onModalBackupTabChange = () => {
    const newTabId = this.state.activeTabId === 0 ? 1 : 0
    this.setState({
      activeTabId: newTabId
    })
  }

  onModalBackupOnCopy = async (backupKey: string) => {
    // TODO(jsadler) possibly flash a message that copy was completed
    try {
      await clipboardCopy(backupKey)
      console.log('Copy successful')
      chrome.send('brave_rewards.setBackupCompleted')
    } catch (e) {
      console.log('Copy failed')
    }
  }

  onModalBackupOnPrint = (backupKey: string) => {
    if (document.location) {
      const win = window.open(document.location.href)
      if (win) {
        win.document.body.innerText = utils.constructBackupString(backupKey) // this should be text, not HTML
        win.print()
        win.close()
        chrome.send('brave_rewards.setBackupCompleted')
      }
    }
  }

  onModalBackupOnSaveFile = (backupKey: string) => {
    const backupString = utils.constructBackupString(backupKey)
    const backupFileText = 'brave_wallet_recovery.txt'
    const a = document.createElement('a')
    document.body.appendChild(a)
    a.style.display = 'display: none'
    const blob = new Blob([backupString], { type : 'plain/text' })
    const url = window.URL.createObjectURL(blob)
    a.href = url
    a.download = backupFileText
    a.click()
    window.URL.revokeObjectURL(url)
    chrome.send('brave_rewards.setBackupCompleted')
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
    this.setState({
      modalActivity: !this.state.modalActivity
    })
  }

  onModalAddFundsToggle = () => {
    this.setState({
      modalAddFunds: !this.state.modalAddFunds
    })
  }

  urlHashIs = (hash: string) => {
    return (
      window &&
      window.location &&
      window.location.hash &&
      window.location.hash === hash
    )
  }

  isAddFundsUrl = () => {
    if (this.urlHashIs('#add-funds')) {
      this.setState({
        modalAddFunds: true
      })
    } else {
      this.setState({
        modalAddFunds: false
      })
    }
  }

  isBackupUrl = () => {
    if (this.urlHashIs('#backup-restore')) {
      this.onModalBackupOpen()
    }
  }

  closeModalAddFunds = () => {
    if (this.urlHashIs('#add-funds')) {
      window.location.hash = ''
    }
    this.onModalAddFundsToggle()
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
    const {
      walletRecoverySuccess,
      walletServerProblem,
      walletCorrupted
    } = this.props.rewardsData.ui

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

    if (walletCorrupted) {
      return {
        node: (
          <>
            <b>{getLocale('uhOh')}</b> {getLocale('walletCorrupted')} <a href={'#'} style={{ 'color': '#838391' }} onClick={this.onModalBackupOpen}>
               {getLocale('walletCorruptedNow')}
             </a>
          </>
        ),
        type: 'error'
      }
    }

    return null
  }

  getWalletSummary = () => {
    const { walletInfo, reports } = this.props.rewardsData
    const { rates } = walletInfo

    let props = {}

    const currentTime = new Date()
    const reportKey = `${currentTime.getFullYear()}_${currentTime.getMonth() + 1}`
    const report: Rewards.Report = reports[reportKey]
    if (report) {
      for (let key in report) {
        const item = report[key]

        if (item.length > 1 && key !== 'total') {
          const tokens = utils.convertProbiToFixed(item)
          props[key] = {
            tokens,
            converted: utils.convertBalance(tokens, rates)
          }
        }
      }
    }

    return {
      report: props
    }
  }

  render () {
    const {
      connectedWallet,
      recoveryKey,
      enabledMain,
      addresses,
      walletInfo,
      ui,
      pendingContributionTotal
    } = this.props.rewardsData
    const { balance } = walletInfo
    const { walletRecoverySuccess, emptyWallet, modalBackup } = ui
    const addressArray = utils.getAddresses(addresses)

    const pendingTotal = parseFloat(
      (pendingContributionTotal || 0).toFixed(1))

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
              : <WalletSummary
                reservedAmount={pendingTotal}
                reservedMoreLink={'https://brave.com/faq-rewards/#unclaimed-funds'}
                {...this.getWalletSummary()}
              />
            : <WalletOff/>
          }
        </WalletWrapper>
        {
          modalBackup
            ? <ModalBackupRestore
              activeTabId={this.state.activeTabId}
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
              onClose={this.closeModalAddFunds}
              addresses={addressArray}
            />
            : null
        }
        {
          // TODO NZ add actual data for the whole section
          this.state.modalActivity
            ? <ModalActivity
              contributeRows={[
                {
                  profile: {
                    name: 'Bart Baker',
                    verified: true,
                    provider: 'youtube',
                    src: ''
                  },
                  url: 'https://brave.com',
                  attention: 40,
                  onRemove: this.onModalActivityRemove,
                  token: {
                    value: '5.0',
                    converted: '5.00'
                  }
                }
              ]}
              transactionRows={[
                {
                  date: '6/1',
                  type: 'deposit',
                  description: 'Brave Ads payment for May',
                  amount: {
                    value: '5.0',
                    converted: '5.00'
                  }
                }
              ]}
              onClose={this.onModalActivityToggle}
              onPrint={this.onModalActivityAction.bind('onPrint')}
              onDownloadPDF={this.onModalActivityAction.bind('onDownloadPDF')}
              onMonthChange={this.onModalActivityAction.bind('onMonthChange')}
              months={{
                'aug-2018': 'August 2018',
                'jul-2018': 'July 2018',
                'jun-2018': 'June 2018',
                'may-2018': 'May 2018',
                'apr-2018': 'April 2018'
              }}
              currentMonth={'aug-2018'}
              summary={[
                {
                  text: 'Token Grant available',
                  type: 'grant',
                  token: {
                    value: '10.0',
                    converted: '5.20'
                  }
                },
                {
                  text: 'Earnings from Brave Ads',
                  type: 'ads',
                  token: {
                    value: '10.0',
                    converted: '5.20'
                  }
                },
                {
                  text: 'Brave Contribute',
                  type: 'contribute',
                  notPaid: true,
                  token: {
                    value: '10.0',
                    converted: '5.20',
                    isNegative: true
                  }
                },
                {
                  text: 'Recurring Donations',
                  type: 'recurring',
                  notPaid: true,
                  token: {
                    value: '2.0',
                    converted: '1.1',
                    isNegative: true
                  }
                },
                {
                  text: 'One-time Donations/Tips',
                  type: 'donations',
                  token: {
                    value: '19.0',
                    converted: '10.10',
                    isNegative: true
                  }
                }
              ]}
              total={{
                value: '1.0',
                converted: '0.5'
              }}
              paymentDay={12}
              openBalance={{
                value: '10.0',
                converted: '5.20'
              }}
              closingBalance={{
                value: '11.0',
                converted: '5.30'
              }}
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
