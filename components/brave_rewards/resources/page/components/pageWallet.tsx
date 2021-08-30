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
  ModalQRCode
} from '../../ui/components'
import { WalletCard, ExternalWalletAction } from '../../shared/components/wallet_card'
import { ExternalWallet, ExternalWalletProvider, ExternalWalletStatus } from '../../shared/lib/external_wallet'
import { WalletAddIcon, WalletWithdrawIcon } from 'brave-ui/components/icons'
import { AlertWallet } from '../../ui/components/walletWrapper'
import { Provider } from '../../ui/components/profile'
import { DetailRow as PendingDetailRow, PendingType } from '../../ui/components/tablePending'
// Utils
import { getLocale, getLocaleWithTag } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
import { ExtendedActivityRow, SummaryItem, SummaryType } from '../../ui/components/modalActivity'
import { DetailRow as TransactionRow } from '../../ui/components/tableTransactions'
import { ConnectWalletModal } from './connect_wallet_modal'
import { ManageWalletButton } from './manage_wallet_button'
import { PageWalletWrapper } from './style'

interface State {
  activeTabId: number
  modalActivity: boolean
  modalPendingContribution: boolean
  modalVerify: boolean
  modalQRCode: boolean
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
      modalVerify: false,
      modalQRCode: false
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
    this.isDisconnectUrl()
    this.isVerifyUrl()
    this.actions.getMonthlyReportIds()
    chrome.send('brave_rewards.getExternalWalletProviders')
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

  isDisconnectUrl = () => {
    if (this.urlHashIs('#disconnect-wallet')) {
      this.actions.disconnectWallet()
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

  toggleQRCodeModal = () => {
    // If we are opening the QR code panel, then request a payment ID if we do
    // not already have one and close the backup/restore modal.
    if (!this.state.modalQRCode) {
      if (!this.props.rewardsData.paymentId) {
        this.actions.getPaymentId()
      }
      this.actions.onModalBackupClose()
    }

    this.setState({
      modalQRCode: !this.state.modalQRCode
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
      disconnectWalletError,
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

    const { externalWallet } = this.props.rewardsData
    if (externalWallet && disconnectWalletError) {
      return {
        node: <><b>{getLocale('uhOh')}</b><br />{getLocale('disconnectWalletFailed').replace('$1', utils.getWalletProviderName(externalWallet))}<br /><br /><a href='https://support.brave.com/hc/en-us/articles/360062026432'>{getLocale('learnMore')}</a></>,
        type: 'error',
        onAlertClose: () => {
          this.actions.onClearAlert('disconnectWalletError')
        }
      }
    }

    return null
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

      if (item.type === 8) { // one-time tip
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
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet) {
      return
    }

    if (externalWallet.status === 0) {
      this.toggleVerifyModal()
      return
    }

    if (externalWallet.status === 1) {
      window.open(externalWallet.verifyUrl, '_self')
      return
    }

    if (externalWallet.loginUrl) {
      window.open(externalWallet.loginUrl, '_self')
    }
  }

  onConnectWalletContinue = (provider: string) => {
    chrome.send('brave_rewards.setExternalWalletType', [provider])
  }

  onVerifyClick = () => {
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet || !externalWallet.loginUrl) {
      this.actions.getExternalWallet()
      return
    }

    this.handleExternalWalletLink()
  }

  getExternalWalletStatus = (): ExternalWalletStatus | null => {
    const { externalWallet } = this.props.rewardsData
    if (!externalWallet) {
      return null
    }

    switch (externalWallet.status) {
      // ledger::type::WalletStatus::CONNECTED
      case 1:
      // WalletStatus::VERIFIED
      case 2:
        return 'verified'
      // WalletStatus::DISCONNECTED_NOT_VERIFIED
      case 3:
      // WalletStatus::DISCONNECTED_VERIFIED
      case 4:
        return 'disconnected'
      // ledger::type::WalletStatus::PENDING
      case 5:
        return 'pending'
      default:
        return null
    }
  }

  getExternalWalletProvider = (): ExternalWalletProvider | null => {
    const { externalWallet } = this.props.rewardsData
    if (!externalWallet) {
      return null
    }

    switch (externalWallet.type) {
      case 'bitflyer': return 'bitflyer'
      case 'gemini': return 'gemini'
      case 'uphold': return 'uphold'
      default: return null
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

    if (externalWallet.loginUrl) {
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
      case 5: { // Rewards.Processor.GEMINI
        text = getLocale('processorGemini')
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
      `${new Date().getFullYear()}_${new Date().getMonth() + 1}`,
      ...monthlyReportIds || []
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

      // Don't show drop-down items for a future month
      if (new Date(year, month).getTime() > Date.now()) {
        return
      }

      const date = new Date(Date.UTC(year, month, 10))
      result[id] = new Intl.DateTimeFormat('default', { month: 'long', year: 'numeric' }).format(date)
    })

    return result
  }

  generateMonthlyReport = () => {
    const { monthlyReport, externalWallet } = this.props.rewardsData

    if (!monthlyReport || monthlyReport.year === -1 || monthlyReport.month === -1) {
      return undefined
    }

    return (
      <ModalActivity
        summary={this.generateSummaryRows()}
        activityRows={this.generateActivityRows()}
        transactionRows={this.generateTransactionRows()}
        months={this.getMonthlyReportDropDown()}
        walletType={externalWallet ? externalWallet.type : ''}
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

  generateExternalWalletProviderList = (walletProviders: string[]) => {
    return walletProviders.map((type) => ({ type, name: utils.getWalletProviderName(type) }))
  }

  onExternalWalletAction = (action: ExternalWalletAction) => {
    switch (action) {
      case 'add-funds':
        this.onFundsAction('add')
        break
      case 'complete-verification':
        this.handleExternalWalletLink()
        break
      case 'disconnect':
        this.onDisconnectClick()
        break
      case 'reconnect':
        this.handleExternalWalletLink()
        break
      case 'verify':
        this.onVerifyClick()
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
      externalWalletProviderList,
      ui,
      recoveryKey,
      externalWallet,
      parameters,
      paymentId
    } = this.props.rewardsData
    const { total } = balance
    const { modalBackup } = ui

    const walletProviderName = utils.getWalletProviderName(externalWallet)

    let externalWalletInfo: ExternalWallet | null = null
    const walletStatus = this.getExternalWalletStatus()
    const walletProvider = this.getExternalWalletProvider()
    if (externalWallet && walletStatus && walletProvider) {
      externalWalletInfo = {
        provider: walletProvider,
        status: walletStatus,
        username: externalWallet.userName || '',
        links: {}
      }
    }

    const summaryData = {
      adEarnings: balanceReport && balanceReport.ads || 0,
      autoContributions: balanceReport && balanceReport.contribute || 0,
      oneTimeTips: balanceReport && balanceReport.tips || 0,
      monthlyTips: balanceReport && balanceReport.monthly || 0
    }

    return (
      <PageWalletWrapper>
        <WalletCard
          balance={total}
          externalWallet={externalWalletInfo}
          earningsThisMonth={adsData.adsEarningsThisMonth || 0}
          earningsLastMonth={adsData.adsEarningsLastMonth || 0}
          nextPaymentDate={0}
          exchangeRate={parameters.rate}
          exchangeCurrency={'USD'}
          showSummary={true}
          summaryData={summaryData}
          onExternalWalletAction={this.onExternalWalletAction}
          onViewStatement={this.onModalActivityToggle}
        />
        <ManageWalletButton onClick={this.onModalBackupOpen} />
        {
          modalBackup
            ? <ModalBackupRestore
              activeTabId={this.state.activeTabId}
              backupKey={recoveryKey}
              showBackupNotice={this.showBackupNotice()}
              walletProvider={walletProviderName}
              onTabChange={this.onModalBackupTabChange}
              onClose={this.onModalBackupClose}
              onCopy={this.onModalBackupOnCopy}
              onPrint={this.onModalBackupOnPrint}
              onSaveFile={this.onModalBackupOnSaveFile}
              onRestore={this.onModalBackupOnRestore}
              onVerify={this.onVerifyClick}
              onReset={this.onModalBackupOnReset}
              onShowQRCode={this.toggleQRCodeModal}
              internalFunds={this.getInternalFunds()}
              error={this.getBackupErrorMessage()}
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
          this.state.modalVerify
            ? <ConnectWalletModal
                rewardsBalance={balance.total}
                providers={this.generateExternalWalletProviderList(externalWalletProviderList)}
                onContinue={this.onConnectWalletContinue}
                onClose={this.toggleVerifyModal}
            />
            : null
        }
        {
          this.state.modalActivity
            ? this.generateMonthlyReport()
            : null
        }
        {
          this.state.modalQRCode
          ? <ModalQRCode
              paymentId={paymentId}
              onClose={this.toggleQRCodeModal}
          />
          : null
        }
      </PageWalletWrapper>
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
