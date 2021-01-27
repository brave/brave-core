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
import { getLocale, getLocaleWithTag } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
import { ExtendedActivityRow, SummaryItem, SummaryType } from '../../ui/components/modalActivity'
import { DetailRow as TransactionRow } from '../../ui/components/tableTransactions'

interface State {
  activeTabId: number
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
      modalActivity: false,
      modalPendingContribution: false,
      modalVerify: false
    }
  }

  get actions () {
    return this.props.actions
  }

  hasUserFunds () {
    const { balance } = this.props.rewardsData
    return balance && balance.wallets['anonymous'] > 0
  }

  componentDidMount () {
    this.isBackupUrl()
    this.isVerifyUrl()
    this.actions.getMonthlyReportIds()
  }

  onModalBackupClose = () => {
    if (this.urlHashIs('#manage-wallet')) {
      window.location.hash = ''
    }
    this.actions.onModalBackupClose()
  }

  onModalBackupOpen = () => {
    if (!this.props.rewardsData.recoveryKey || this.props.rewardsData.recoveryKey.length === 0) {
      this.actions.getWalletPassphrase()
    }
    this.actions.onModalBackupOpen()
  }

  showBackupNotice = () => {
    return this.state.activeTabId === 0 && !this.hasUserFunds()
  }

  onModalBackupTabChange = (newTabId: number) => {
    this.setState({
      activeTabId: newTabId
    })
  }

  onModalBackupOnCopy = async (backupKey: string) => {
    // TODO(jsadler) possibly flash a message that copy was completed
    try {
      await navigator.clipboard.writeText(backupKey)
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

  onModalBackupOnReset = () => {
    this.actions.onModalBackupClose()
    this.actions.completeReset()
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
    const { balance, parameters } = this.props.rewardsData
    return utils.convertBalance(balance.total, parameters.rate)
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
    if (this.urlHashIs('#manage-wallet')) {
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
    const {
      walletRecoveryStatus,
      walletServerProblem
    } = this.props.rewardsData.ui

    if (walletServerProblem) {
      return {
        node: <><b>{getLocale('uhOh')}</b> {getLocale('serverNotResponding')}</>,
        type: 'error'
      }
    }

    if (walletRecoveryStatus === 0) {
      return {
        node: <><b>{getLocale('walletRestored')}</b> {getLocale('walletRecoverySuccess')}</>,
        type: 'success',
        onAlertClose: () => {
          this.actions.onClearAlert('walletRecoveryStatus')
        }
      }
    }

    return null
  }

  getWalletSummary = () => {
    const { parameters, balanceReport, pendingContributionTotal } = this.props.rewardsData

    let props = {}

    if (balanceReport) {
      for (let key in balanceReport) {
        const item = balanceReport[key]

        if (item !== 0) {
          const tokens = item.toFixed(3)
          props[key] = {
            tokens,
            converted: utils.convertBalance(item, parameters.rate)
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
    const { parameters, pendingContributions } = this.props.rewardsData
    return pendingContributions.map((item: Rewards.PendingContribution) => {
      const verified = utils.isPublisherConnectedOrVerified(item.status)
      let faviconUrl = `chrome://favicon/size/64@1x/${item.url}`
      if (item.favIcon && verified) {
        faviconUrl = `chrome://favicon/size/64@1x/${item.favIcon}`
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
          tokens: item.amount.toFixed(3),
          converted: utils.convertBalance(item.amount, parameters.rate)
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

  handleExternalWalletLink = () => {
    const { ui, externalWallet, balance } = this.props.rewardsData

    if (!externalWallet) {
      return
    }

    if (balance.total < 25 && externalWallet.type === 'uphold') {
      window.open(externalWallet.loginUrl, '_self')
      return
    }

    if (!ui.verifyOnboardingDisplayed && externalWallet.status === 0) {
      this.toggleVerifyModal()
      return
    }

    window.open(externalWallet.verifyUrl, '_self')
  }

  onVerifyClick = (hideVerify: boolean) => {
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet || !externalWallet.verifyUrl) {
      this.actions.getExternalWallet()
      return
    }

    if (hideVerify) {
      this.actions.onVerifyOnboardingDisplayed()
    }

    this.handleExternalWalletLink()
  }

  getWalletStatus = (): WalletState | undefined => {
    const { externalWallet, ui } = this.props.rewardsData

    if (ui.onlyAnonWallet || !externalWallet) {
      return undefined
    }

    switch (externalWallet.status) {
      // ledger::type::WalletStatus::CONNECTED
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
      // ledger::type::WalletStatus::PENDING
      case 5:
        return 'pending'
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
      this.handleExternalWalletLink()
      return
    }
  }

  goToExternalWallet = () => {
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet || !externalWallet.accountUrl) {
      this.actions.getExternalWallet()
      return
    }

    window.open(externalWallet.accountUrl, '_self')
  }

  getGreetings = () => {
    const { externalWallet } = this.props.rewardsData
    if (!externalWallet || !externalWallet.userName) {
      return ''
    }

    return getLocale('greetingsVerified', { name: externalWallet.userName })
  }

  onDisconnectClick = () => {
    this.actions.disconnectWallet()
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
        testId: 'panel-add-funds',
        externalWallet: true
      },
      {
        name: getLocale('panelWithdrawFunds'),
        action: this.onFundsAction.bind(this, 'withdraw'),
        icon: <WalletWithdrawIcon />,
        testId: 'panel-withdraw-funds',
        externalWallet: true
      }
    ]
  }

  getBalanceToken = (key: string) => {
    const {
      monthlyReport,
      parameters
    } = this.props.rewardsData

    let value = 0.0
    if (monthlyReport && monthlyReport.balance && monthlyReport.balance[key]) {
      value = monthlyReport.balance[key]
    }

    return {
      value: value.toFixed(3),
      converted: utils.convertBalance(value, parameters.rate)
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
      parameters
    } = this.props.rewardsData

    if (!monthlyReport.contributions) {
      return []
    }

    let records: ExtendedActivityRow[] = []

    monthlyReport.contributions
      .forEach((contribution: Rewards.ContributionReport) => {
        records = contribution.publishers
          .map((publisher: Rewards.Publisher): ExtendedActivityRow => {
            let faviconUrl = `chrome://favicon/size/64@1x/${publisher.url}`
            const verified = utils.isPublisherConnectedOrVerified(publisher.status)
            if (publisher.favIcon && verified) {
              faviconUrl = `chrome://favicon/size/64@1x/${publisher.favIcon}`
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
                tokens: publisher.weight.toFixed(3),
                converted: utils.convertBalance(publisher.weight, parameters.rate)
              },
              type: this.getSummaryType(contribution.type),
              date: contribution.created_at
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

  getProcessorString = (processor: Rewards.Processor) => {
    let text = ''
    switch (processor) {
      case 0: { // Rewards.Processor.NONE
        text = ''
        break
      }
      case 1: { // Rewards.Processor.BRAVE_TOKENS
        text = getLocale('processorBraveTokens')
        break
      }
      case 2: { // Rewards.Processor.UPHOLD
        text = getLocale('processorUphold')
        break
      }
      case 3: { // Rewards.Processor.BRAVE_USER_FUNDS
        text = getLocale('processorBraveUserFunds')
        break
      }
      case 4: { // Rewards.Processor.BITFLYER
        text = getLocale('processorBitflyer')
        break
      }
    }

    if (text.length === 0) {
      return ''
    }

    return `(${text})`
  }

  getContributionDescription = (contribution: Rewards.ContributionReport) => {
    if (contribution.type === 1) { // Rewards.ReportType.AUTO_CONTRIBUTION
      return getLocale(
        'autoContributeTransaction',
        { processor: this.getProcessorString(contribution.processor) })
    }

    return ''
  }

  generateTransactionRows = (): TransactionRow[] => {
    const {
      monthlyReport,
      parameters
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
            value: transaction.amount.toFixed(3),
            converted: utils.convertBalance(transaction.amount, parameters.rate)
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
                value: contribution.amount.toFixed(3),
                converted: utils.convertBalance(contribution.amount, parameters.rate),
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

    const ids = [
      ...monthlyReportIds || [],
      `${new Date().getFullYear()}_${new Date().getMonth() + 1}`
    ]

    let result: Record<string, string> = {}
    ids.forEach((id: string) => {
      const items = id.split('_')
      if (items.length !== 2) {
        return
      }

      const year = parseInt(items[0], 10)
      const month = parseInt(items[1], 10) - 1

      // we only want to show reports from version 1.3 (Feb 2020) up
      if (year < 2020 || (year === 2020 && month === 0)) {
        return
      }

      const date = new Date(Date.UTC(year, month, 10))
      result[id] = new Intl.DateTimeFormat('default', { month: 'long', year: 'numeric' }).format(date)
    })

    return result
  }

  generateMonthlyReport = () => {
    const { monthlyReport, ui } = this.props.rewardsData

    if (!monthlyReport || monthlyReport.year === -1 || monthlyReport.month === -1) {
      return undefined
    }

    return (
      <ModalActivity
        onlyAnonWallet={ui.onlyAnonWallet}
        summary={this.generateSummaryRows()}
        activityRows={this.generateActivityRows()}
        transactionRows={this.generateTransactionRows()}
        months={this.getMonthlyReportDropDown()}
        onClose={this.onModalActivityToggle}
        onMonthChange={this.onModalActivityAction.bind(this,'onMonthChange')}
      />
    )
  }

  getInternalFunds = () => {
    const { balance } = this.props.rewardsData
    if (!balance.wallets) {
      return 0
    }

    return (balance.wallets['anonymous'] || 0) + (balance.wallets['blinded'] || 0)
  }

  getBackupErrorMessage = () => {
    const { ui } = this.props.rewardsData
    const { walletRecoveryStatus } = ui

    if (walletRecoveryStatus === null) {
      return ''
    }

    // ledger::type::Result::CORRUPTED_DATA
    if (walletRecoveryStatus === 17) {
      const tags = getLocaleWithTag('walletRecoveryOutdated')
      return (
        <span>
          {tags.beforeTag}
          <a href='https://brave.com/faq#convert-old-keys' target='_blank' rel='noopener noreferrer'>
            {tags.duringTag}
          </a>
          {tags.afterTag}
        </span>
      )
    }

    if (walletRecoveryStatus !== 0) {
      return getLocale('walletRecoveryFail')
    }

    return ''
  }

  showLoginMessage = () => {
    const { balance, externalWallet } = this.props.rewardsData
    const walletStatus = this.getWalletStatus()
    const walletType = externalWallet ? externalWallet.type : ''

    return (
      (!walletStatus || walletStatus === 'unverified') &&
      walletType === 'uphold' &&
      balance &&
      balance.total < 25
    )
  }

  render () {
    const {
      balance,
      ui,
      pendingContributionTotal,
      recoveryKey,
      externalWallet
    } = this.props.rewardsData
    const { total } = balance
    const { emptyWallet, modalBackup, onlyAnonWallet } = ui

    const pendingTotal = parseFloat((pendingContributionTotal || 0).toFixed(3))
    const walletType = externalWallet ? externalWallet.type : undefined
    const walletProvider = utils.getWalletProviderName(externalWallet)

    let onVerifyClick = undefined
    let showCopy = false
    if (!onlyAnonWallet) {
      onVerifyClick = this.onVerifyClick.bind(this, false)
      showCopy = true
    }

    return (
      <>
        <WalletWrapper
          balance={total.toFixed(3)}
          converted={utils.formatConverted(this.getConversion())}
          actions={this.getActions()}
          onSettingsClick={this.onModalBackupOpen}
          showCopy={showCopy}
          showSecActions={true}
          alert={this.walletAlerts()}
          walletType={walletType}
          walletState={this.getWalletStatus()}
          walletProvider={walletProvider}
          onVerifyClick={onVerifyClick}
          onDisconnectClick={this.onDisconnectClick}
          goToExternalWallet={this.goToExternalWallet}
          greetings={this.getGreetings()}
          onlyAnonWallet={onlyAnonWallet}
          showLoginMessage={this.showLoginMessage()}
        >
          {
            emptyWallet && pendingTotal === 0
            ? <WalletEmpty onlyAnonWallet={onlyAnonWallet} />
            : <WalletSummary
              reservedAmount={pendingTotal}
              onlyAnonWallet={onlyAnonWallet}
              reservedMoreLink={'https://brave.com/faq/#unclaimed-funds'}
              onActivity={this.onModalActivityToggle}
              {...this.getWalletSummary()}
            />
          }
        </WalletWrapper>
        {
          modalBackup
            ? <ModalBackupRestore
              activeTabId={this.state.activeTabId}
              backupKey={recoveryKey}
              showBackupNotice={this.showBackupNotice()}
              walletProvider={walletProvider}
              onTabChange={this.onModalBackupTabChange}
              onClose={this.onModalBackupClose}
              onCopy={this.onModalBackupOnCopy}
              onPrint={this.onModalBackupOnPrint}
              onSaveFile={this.onModalBackupOnSaveFile}
              onRestore={this.onModalBackupOnRestore}
              onVerify={this.onVerifyClick.bind(this, true)}
              onReset={this.onModalBackupOnReset}
              internalFunds={this.getInternalFunds()}
              error={this.getBackupErrorMessage()}
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
              walletType={walletType}
              walletProvider={walletProvider}
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
