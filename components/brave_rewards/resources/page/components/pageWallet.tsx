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
  ModalPending,
  ModalVerify,
  WalletEmpty,
  WalletSummary,
  WalletWrapper
} from '../../ui/components'
import { WalletAddIcon, WalletWithdrawIcon } from 'brave-ui/components/icons'
import { AlertWallet, WalletState } from '../../ui/components/walletWrapper'
import { Provider } from '../../ui/components/profile'
import { DetailRow as PendingDetailRow, PendingType } from '../../ui/components/tablePending'
// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
import WalletOff from '../../ui/components/walletOff'

const clipboardCopy = require('clipboard-copy')

interface State {
  activeTabId: number
  modalBackup: boolean
  modalActivity: boolean
  modalPendingContribution: boolean
  modalVerify: boolean
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
      modalPendingContribution: false,
      modalVerify: false
    }
  }

  get actions () {
    return this.props.actions
  }

  componentDidMount () {
    this.isBackupUrl()
    this.isVerifyUrl()
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
    const balance = this.props.rewardsData.balance
    return utils.convertBalance(balance.total.toString(), balance.rates)
  }

  getGrants = () => {
    const grants = this.props.rewardsData.walletInfo.grants
    if (!grants) {
      return []
    }

    return grants.map((grant: Rewards.Grant) => {
      return {
        tokens: utils.convertProbiToFixed(grant.probi),
        expireDate: new Date(grant.expiryTime * 1000).toLocaleDateString(),
        type: grant.type || 'ugp'
      }
    })
  }

  onModalActivityToggle = () => {
    this.setState({
      modalActivity: !this.state.modalActivity
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

  onModalPendingToggle = () => {
    this.setState({
      modalPendingContribution: !this.state.modalPendingContribution
    })
  }

  isBackupUrl = () => {
    if (this.urlHashIs('#backup-restore')) {
      this.onModalBackupOpen()
    }
  }

  isVerifyUrl = () => {
    if (this.urlHashIs('#verify')) {
      this.toggleVerifyModal()
    }
  }

  toggleVerifyModal = () => {
    if (this.state.modalVerify) {
      window.history.replaceState({}, 'Rewards', '/')
    }
    this.setState({
      modalVerify: !this.state.modalVerify
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
    const { total } = this.props.rewardsData.balance
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
        node: <><b>{getLocale('walletRestored')}</b> {getLocale('walletRecoverySuccess', { balance: total.toString() })}</>,
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
    const { balance, reports, pendingContributionTotal } = this.props.rewardsData
    const { rates } = balance

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

    let result: {report: Record<string, {tokens: string, converted: string}>, onSeeAllReserved?: () => {}} = {
      report: props,
      onSeeAllReserved: undefined
    }

    if (pendingContributionTotal > 0) {
      result.onSeeAllReserved = this.onModalPendingToggle.bind(this)
    }

    return result
  }

  getPendingRows = (): PendingDetailRow[] => {
    const { balance, pendingContributions } = this.props.rewardsData
    return pendingContributions.map((item: Rewards.PendingContribution) => {
      const verified = utils.isPublisherConnectedOrVerified(item.status)
      let faviconUrl = `chrome://favicon/size/48@1x/${item.url}`
      if (item.favIcon && verified) {
        faviconUrl = `chrome://favicon/size/48@1x/${item.favIcon}`
      }

      let type: PendingType = 'ac'

      if (item.type === 8) { // one time tip
        type = 'tip'
      } else if (item.type === 16) { // recurring tip
        type = 'recurring'
      }

      return {
        profile: {
          name: item.name,
          verified,
          provider: (item.provider ? item.provider : undefined) as Provider,
          src: faviconUrl
        },
        url: item.url,
        type,
        amount: {
          tokens: item.amount.toFixed(1),
          converted: utils.convertBalance(item.amount.toString(), balance.rates)
        },
        date: new Date(parseInt(item.expirationDate, 10) * 1000).toLocaleDateString(),
        onRemove: () => {
          this.actions.removePendingContribution(item.publisherKey, item.viewingId, item.addedDate)
        }
      }
    })
  }

  removeAllPendingContribution = () => {
    this.actions.removeAllPendingContribution()
  }

  handleUpholdLink = (link: string) => {
    const { ui, externalWallet } = this.props.rewardsData
    if (!ui.onBoardingDisplayed &&
        (!externalWallet || (externalWallet && externalWallet.status === 0))) {
      this.toggleVerifyModal()
      return
    }

    window.open(link, '_self')
  }

  onVerifyClick = (hideVerify: boolean) => {
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet || !externalWallet.verifyUrl) {
      this.actions.getExternalWallet('uphold')
      return
    }

    if (hideVerify) {
      this.actions.onOnBoardingDisplayed()
    }

    this.handleUpholdLink(externalWallet.verifyUrl)
  }

  getWalletStatus = (): WalletState | undefined => {
    const { externalWallet, ui } = this.props.rewardsData

    if (ui.onlyAnonWallet) {
      return undefined
    }

    if (!externalWallet) {
      return 'unverified'
    }

    switch (externalWallet.status) {
      // ledger::WalletStatus::CONNECTED
      case 1:
        return 'connected'
      // WalletStatus::VERIFIED
      case 2:
        return 'verified'
      // WalletStatus::DISCONNECTED_NOT_VERIFIED
      case 3:
        return 'disconnected_unverified'
      // WalletStatus::DISCONNECTED_VERIFIED
      case 4:
        return 'disconnected_verified'
      default:
        return 'unverified'
    }
  }

  onFundsAction = (action: string) => {
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet) {
      return
    }

    switch (action) {
      case 'add': {
        if (externalWallet.addUrl) {
          window.open(externalWallet.addUrl, '_self')
          return
        }
        break
      }
      case 'withdraw': {
        if (externalWallet.withdrawUrl) {
          window.open(externalWallet.withdrawUrl, '_self')
          return
        }
        break
      }
    }

    if (externalWallet.verifyUrl) {
      this.handleUpholdLink(externalWallet.verifyUrl)
      return
    }
  }

  goToUphold = () => {
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet || !externalWallet.accountUrl) {
      this.actions.getExternalWallet('uphold')
      return
    }

    window.open(externalWallet.accountUrl, '_self')
  }

  getUserName = () => {
    const { externalWallet } = this.props.rewardsData
    if (!externalWallet) {
      return ''
    }

    return externalWallet.userName
  }

  onDisconnectClick = () => {
    this.actions.disconnectWallet('uphold')
  }

  getActions = () => {
    const { ui } = this.props.rewardsData

    if (ui.onlyAnonWallet) {
      return []
    }

    return [
      {
        name: getLocale('panelAddFunds'),
        action: this.onFundsAction.bind(this, 'add'),
        icon: <WalletAddIcon />,
        testId: 'panel-add-funds'
      },
      {
        name: getLocale('panelWithdrawFunds'),
        action: this.onFundsAction.bind(this, 'withdraw'),
        icon: <WalletWithdrawIcon />,
        testId: 'panel-withdraw-funds'
      }
    ]
  }

  render () {
    const {
      recoveryKey,
      enabledMain,
      balance,
      ui,
      pendingContributionTotal
    } = this.props.rewardsData
    const { total } = balance
    const { walletRecoverySuccess, emptyWallet, modalBackup, onlyAnonWallet } = ui

    const pendingTotal = parseFloat((pendingContributionTotal || 0).toFixed(1))

    let onVerifyClick = undefined
    let showCopy = false
    if (!onlyAnonWallet) {
      onVerifyClick = this.onVerifyClick.bind(this, false)
      showCopy = true
    }

    return (
      <>
        <WalletWrapper
          balance={total.toFixed(1)}
          converted={utils.formatConverted(this.getConversion())}
          actions={this.getActions()}
          onSettingsClick={this.onModalBackupOpen}
          onActivityClick={this.onModalActivityToggle}
          showCopy={showCopy}
          showSecActions={true}
          grants={this.getGrants()}
          alert={this.walletAlerts()}
          walletState={this.getWalletStatus()}
          onVerifyClick={onVerifyClick}
          onDisconnectClick={this.onDisconnectClick}
          goToUphold={this.goToUphold}
          userName={this.getUserName()}
        >
          {
            enabledMain
            ? emptyWallet
              ? <WalletEmpty />
              : <WalletSummary
                reservedAmount={pendingTotal}
                reservedMoreLink={'https://brave.com/faq/#unclaimed-funds'}
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
          this.state.modalPendingContribution
            ? <ModalPending
              onClose={this.onModalPendingToggle}
              rows={this.getPendingRows()}
              onRemoveAll={this.removeAllPendingContribution}
            />
            : null
        }
        {
          !onlyAnonWallet && this.state.modalVerify
            ? <ModalVerify
              onVerifyClick={this.onVerifyClick.bind(this, true)}
              onClose={this.toggleVerifyModal}
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
                  text: 'Recurring Tips',
                  type: 'recurring',
                  notPaid: true,
                  token: {
                    value: '2.0',
                    converted: '1.1',
                    isNegative: true
                  }
                },
                {
                  text: 'One-timen Tips',
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
