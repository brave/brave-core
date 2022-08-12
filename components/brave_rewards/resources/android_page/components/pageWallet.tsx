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
  ModalQRCode
} from '../../ui/components'
import { PendingContributionsModal } from './pending_contributions_modal'
import { WalletCard, ExternalWalletAction } from '../../shared/components/wallet_card'
import { getProviderPayoutStatus } from '../../shared/lib/provider_payout_status'
import { ExternalWallet, ExternalWalletProvider, ExternalWalletStatus } from '../../shared/lib/external_wallet'
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
  showManageWalletButton: boolean
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

  onModalBackupTabChange = (newTabId: number) => {
    this.setState({
      activeTabId: newTabId
    })
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
    }
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

  onDisconnectClick = () => {
    this.actions.disconnectWallet()
  }

  getBalanceToken = (key: string) => {
    const {
      monthlyReport,
      parameters,
      externalWallet
    } = this.props.rewardsData

    let value = 0.0
    if (monthlyReport && monthlyReport.balance && monthlyReport.balance[key]) {
      value = monthlyReport.balance[key]
    }

    return {
      value: value.toFixed(3),
      converted: utils.convertBalance(value, parameters.rate),
      link: externalWallet && externalWallet.status === 2 /* VERIFIED */ && key === 'ads' ? externalWallet.activityUrl : undefined
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
        onMonthChange={this.onModalActivityAction.bind(this, 'onMonthChange')}
      />
    )
  }

  getInternalFunds = () => {
    const { balance } = this.props.rewardsData
    if (!balance.wallets) {
      return 0
    }

    return (balance.wallets.blinded || 0)
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
      enabledContribute,
      externalWalletProviderList,
      ui,
      externalWallet,
      parameters,
      paymentId,
      pendingContributionTotal
    } = this.props.rewardsData
    const { total } = balance
    const { modalBackup } = ui

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

    const providerPayoutStatus = getProviderPayoutStatus(
      parameters.payoutStatus,
      walletProvider && walletStatus ? walletProvider : null)

    const summaryData = {
      adEarnings: balanceReport && balanceReport.ads || 0,
      autoContributions: balanceReport && balanceReport.contribute || 0,
      oneTimeTips: balanceReport && balanceReport.tips || 0,
      monthlyTips: balanceReport && balanceReport.monthly || 0,
      pendingTips: pendingContributionTotal || 0
    }

    return (
      <PageWalletWrapper>
        <WalletCard
          balance={total}
          externalWallet={externalWalletInfo}
          providerPayoutStatus={providerPayoutStatus}
          earningsThisMonth={adsData.adsEarningsThisMonth || 0}
          earningsLastMonth={adsData.adsEarningsLastMonth || 0}
          nextPaymentDate={0}
          exchangeRate={parameters.rate}
          exchangeCurrency={'USD'}
          showSummary={true}
          summaryData={summaryData}
          autoContributeEnabled={enabledContribute}
          onExternalWalletAction={this.onExternalWalletAction}
          onViewPendingTips={this.onModalPendingToggle}
        />
        { this.props.showManageWalletButton && <ManageWalletButton onClick={this.onModalBackupOpen} /> }
        {
          modalBackup
            ? <ModalBackupRestore
              activeTabId={this.state.activeTabId}
              onTabChange={this.onModalBackupTabChange}
              onClose={this.onModalBackupClose}
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
            ? <PendingContributionsModal
              onClose={this.onModalPendingToggle}
              rows={this.getPendingRows()}
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
