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
import { ExtendedActivityRow, SummaryItem, SummaryType } from '../../ui/components/modalActivity'
import { DetailRow as TransactionRow } from '../../ui/components/tableTransactions'

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
    this.actions.getMonthlyReportIds()
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
    return utils.convertBalance(balance.total, balance.rates)
  }

  generatePromotions = () => {
    const promotions = this.props.rewardsData.promotions
    if (!promotions) {
      return []
    }

    let claimedPromotions = promotions.filter((promotion: Rewards.Promotion) => {
      return promotion.status === 4 // PromotionStatus::FINISHED
    })

    const typeUGP = 0
    return claimedPromotions.map((promotion: Rewards.Promotion) => {
      return {
        amount: promotion.amount,
        expiresAt: new Date(promotion.expiresAt).toLocaleDateString(),
        type: promotion.type || typeUGP
      }
    })
  }

  onModalActivityToggle = () => {
    if (!this.state.modalActivity) {
      this.actions.getMonthlyReport()
    }

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

  onModalActivityAction = (action: string, value: string, node: any) => {
    if (action === 'onMonthChange') {
      const items = value.split('_')
      if (items.length !== 2) {
        return
      }
      this.actions.getMonthlyReport(parseInt(items[1], 10), parseInt(items[0], 10))
      return
    }
  }

  walletAlerts = (): AlertWallet | null => {
    const { total } = this.props.rewardsData.balance
    const {
      walletRecoverySuccess,
      walletServerProblem,
      walletCorrupted,
      onlyAnonWallet
    } = this.props.rewardsData.ui

    if (walletServerProblem) {
      return {
        node: <><b>{getLocale('uhOh')}</b> {getLocale('serverNotResponding')}</>,
        type: 'error'
      }
    }

    if (walletRecoverySuccess) {
      const batFormatString = onlyAnonWallet ? getLocale('batPoints') : getLocale('bat')

      return {
        node: <><b>{getLocale('walletRestored')}</b> {getLocale('walletRecoverySuccess', { balance: total.toString(), currency: batFormatString })}</>,
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
    const { balance, balanceReport, pendingContributionTotal } = this.props.rewardsData
    const { rates } = balance

    let props = {}

    if (balanceReport) {
      for (let key in balanceReport) {
        const item = balanceReport[key]

        if (item !== 0) {
          const tokens = item.toFixed(1)
          props[key] = {
            tokens,
            converted: utils.convertBalance(item, rates)
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
          converted: utils.convertBalance(item.amount, balance.rates)
        },
        date: new Date(parseInt(item.expirationDate, 10) * 1000).toLocaleDateString(),
        onRemove: () => {
          this.actions.removePendingContribution(item.id)
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

  getBalanceToken = (key: string) => {
    const {
      monthlyReport,
      balance
    } = this.props.rewardsData

    let value = 0.0
    if (monthlyReport && monthlyReport.balance && monthlyReport.balance[key]) {
      value = monthlyReport.balance[key]
    }

    return {
      value: value.toFixed(1),
      converted: utils.convertBalance(value, balance.rates)
    }
  }

  generateSummaryRows = (): SummaryItem[] => {
    return [
      {
        type: 'grant',
        token: this.getBalanceToken('grant')
      },
      {
        type: 'ads',
        token: this.getBalanceToken('ads')
      },
      {
        type: 'contribute',
        token: this.getBalanceToken('contribute')
      },
      {
        type: 'monthly',
        token: this.getBalanceToken('monthly')
      },
      {
        type: 'tip',
        token: this.getBalanceToken('tips')
      }
    ]
  }

  generateActivityRows = (): ExtendedActivityRow[] => {
    const {
      monthlyReport,
      balance
    } = this.props.rewardsData

    if (!monthlyReport.contributions) {
      return []
    }

    let records: ExtendedActivityRow[] = []

    monthlyReport.contributions
      .forEach((contribution: Rewards.ContributionReport) => {
        records = contribution.publishers
          .map((publisher: Rewards.Publisher): ExtendedActivityRow => {
            let faviconUrl = `chrome://favicon/size/48@1x/${publisher.url}`
            const verified = utils.isPublisherConnectedOrVerified(publisher.status)
            if (publisher.favIcon && verified) {
              faviconUrl = `chrome://favicon/size/48@1x/${publisher.favIcon}`
            }
            return {
              profile: {
                name: publisher.name,
                verified,
                provider: (publisher.provider ? publisher.provider : undefined) as Provider,
                src: faviconUrl
              },
              url: publisher.url,
              amount: {
                tokens: publisher.weight.toFixed(1),
                converted: utils.convertBalance(publisher.weight, balance.rates)
              },
              type: this.getSummaryType(contribution.type)
            }
          })
          .concat(records)
      })

    return records
  }

  getSummaryType = (type: Rewards.ReportType): SummaryType => {
    switch (type) {
      case 0: { // Rewards.ReportType.GRANT_UGP
        return 'grant'
      }
      case 1: { // Rewards.ReportType.AUTO_CONTRIBUTION
        return 'contribute'
      }
      case 3: { // Rewards.ReportType.GRANT_AD
        return 'ads'
      }
      case 4: { // Rewards.ReportType.TIP_RECURRING
        return 'monthly'
      }
      case 5: { // Rewards.ReportType.TIP
        return 'tip'
      }
    }

    return 'contribute'
  }

  getTransactionDescription = (transaction: Rewards.TransactionReport) => {
    switch (transaction.type) {
      case 0: { // Rewards.ReportType.GRANT_UGP
        return getLocale('tokenGrantReceived')
      }
      case 3: { // Rewards.ReportType.GRANT_AD
        return getLocale('adsGrantReceived')
      }
    }

    return ''
  }

  getContributionDescription = (contribution: Rewards.ContributionReport) => {
    if (contribution.type === 1) { // Rewards.ReportType.AUTO_CONTRIBUTION
      return getLocale('autoContribute')
    }

    return ''
  }

  generateTransactionRows = (): TransactionRow[] => {
    const {
      monthlyReport,
      balance
    } = this.props.rewardsData

    if (!monthlyReport.transactions && !monthlyReport.contributions) {
      return []
    }

    let transactions: TransactionRow[] = []

    if (monthlyReport.transactions) {
      transactions = monthlyReport.transactions.map((transaction: Rewards.TransactionReport) => {
        return {
          date: transaction.created_at,
          type: this.getSummaryType(transaction.type),
          description: this.getTransactionDescription(transaction),
          amount: {
            value: transaction.amount.toFixed(1),
            converted: utils.convertBalance(transaction.amount, balance.rates)
          }
        }
      })
    }

    if (monthlyReport.contributions) {
      transactions = transactions.concat(
        monthlyReport.contributions
          .filter((contribution: Rewards.ContributionReport) => contribution.type === 1)
          .map((contribution: Rewards.ContributionReport) => {
            return {
              date: contribution.created_at,
              type: this.getSummaryType(contribution.type),
              description: this.getContributionDescription(contribution),
              amount: {
                value: contribution.amount.toFixed(1),
                converted: utils.convertBalance(contribution.amount, balance.rates),
                isNegative: true
              }
            }
          })
      )
    }

    transactions.sort((a, b) => a.date - b.date)

    return transactions
  }

  getMonthlyReportDropDown = (): Record<string, string> => {
    const { monthlyReportIds } = this.props.rewardsData

    let ids = monthlyReportIds
    if (!monthlyReportIds) {
      ids = []
      ids.push(`${new Date().getFullYear()}_${new Date().getMonth() + 1}`)
    }

    let result: Record<string, string> = {}
    ids.forEach((id: string) => {
      const items = id.split('_')
      if (items.length !== 2) {
        return
      }

      const date = new Date(Date.UTC(parseInt(items[0], 10), parseInt(items[1], 10) - 1, 1))
      result[id] = new Intl.DateTimeFormat('default', { month: 'long', year: 'numeric' }).format(date)
    })

    return result
  }

  generateMonthlyReport = () => {
    const {
      monthlyReport,
      ui,
      reconcileStamp
    } = this.props.rewardsData

    if (!monthlyReport || monthlyReport.year === -1 || monthlyReport.month === -1) {
      return undefined
    }

    const paymentDay = new Intl.DateTimeFormat('default', { day: 'numeric' }).format(reconcileStamp * 1000)

    return (
      <ModalActivity
        onlyAnonWallet={ui.onlyAnonWallet}
        summary={this.generateSummaryRows()}
        activityRows={this.generateActivityRows()}
        transactionRows={this.generateTransactionRows()}
        months={this.getMonthlyReportDropDown()}
        onClose={this.onModalActivityToggle}
        onMonthChange={this.onModalActivityAction.bind(this,'onMonthChange')}
        paymentDay={parseInt(paymentDay, 10)}
      />
    )
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
          showCopy={showCopy}
          showSecActions={true}
          grants={this.generatePromotions()}
          alert={this.walletAlerts()}
          walletState={this.getWalletStatus()}
          onVerifyClick={onVerifyClick}
          onDisconnectClick={this.onDisconnectClick}
          goToUphold={this.goToUphold}
          userName={this.getUserName()}
          onlyAnonWallet={onlyAnonWallet}
        >
          {
            enabledMain
            ? emptyWallet && pendingTotal === 0
              ? <WalletEmpty onlyAnonWallet={onlyAnonWallet} />
              : <WalletSummary
                reservedAmount={pendingTotal}
                onlyAnonWallet={onlyAnonWallet}
                reservedMoreLink={'https://brave.com/faq/#unclaimed-funds'}
                onActivity={this.onModalActivityToggle}
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
              onlyAnonWallet={onlyAnonWallet}
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
          this.state.modalActivity
            ? this.generateMonthlyReport()
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
