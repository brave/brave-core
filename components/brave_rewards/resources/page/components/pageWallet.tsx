/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
// Components
import {
  ModalActivity,
  ModalReset
} from '../../ui/components'
import { WalletCard, ExternalWalletAction } from '../../shared/components/wallet_card'
import { LayoutKind } from '../lib/layout_context'

import {
  ExternalWallet,
  ExternalWalletProvider,
  lookupExternalWalletProviderName,
  isExternalWalletProviderAllowed
} from '../../shared/lib/external_wallet'

import { Provider } from '../../ui/components/profile'
// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import { convertBalance } from './utils'
import { ExtendedActivityRow, SummaryItem, SummaryType } from '../../ui/components/modalActivity'
import { DetailRow as TransactionRow } from '../../ui/components/tableTransactions'
import { ConnectWalletModal } from './connect_wallet_modal'

import * as mojom from '../../shared/lib/mojom'
import { isPublisherVerified } from '../../shared/lib/publisher_status'

import * as Rewards from '../lib/types'

interface State {
  modalActivity: boolean
}

interface Props extends Rewards.ComponentProps {
  layout: LayoutKind
}

class PageWallet extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      modalActivity: false
    }
  }

  get actions () {
    return this.props.actions
  }

  componentDidMount () {
    this.isBackupUrl()
    this.isVerifyUrl()
    this.actions.getMonthlyReportIds()
    this.actions.getExternalWalletProviders()
    if (this.props.rewardsData.userType !== 'unconnected') {
      this.isMonthlyStatementsUrl()
    }
  }

  onModalResetClose = () => {
    // Used by the settings page to clear browsing data.
    if (this.urlHashIs('#manage-wallet')) {
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

  onModalActivityToggle = () => {
    if (!this.state.modalActivity) {
      this.actions.getMonthlyReport()
    }

    this.setState({
      modalActivity: !this.state.modalActivity
    })

    if (!this.state.modalActivity && this.urlHashIs('#monthly-statements')) {
      history.replaceState({}, '', '/')
    }
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
    if (this.urlHashIs('#manage-wallet')) {
      this.onModalResetOpen()
    }
  }

  isVerifyUrl = () => {
    if (this.urlHashIs('#verify')) {
      this.toggleVerifyModal()
    }
  }

  isMonthlyStatementsUrl = () => {
    if (this.urlHashIs('#monthly-statements')) {
      this.onModalActivityToggle()
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

  onModalActivityAction = (action: string, value: string, node: any) => {
    if (action === 'onMonthChange') {
      const items = value.split('_')
      if (items.length !== 2) {
        return
      }
      this.actions.getMonthlyReport(parseInt(items[1], 10), parseInt(items[0], 10))
    }
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
      window.open(externalWallet.loginUrl, '_self', 'noreferrer')
    }
  }

  onConnectWalletContinue = (provider: string) => {
    this.actions.setExternalWalletType(provider)
  }

  onVerifyClick = () => {
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet || !externalWallet.loginUrl) {
      this.actions.getExternalWallet()
      return
    }

    this.handleExternalWalletLink()
  }

  getExternalWalletStatus = (): mojom.WalletStatus | null => {
    const { externalWallet } = this.props.rewardsData
    if (!externalWallet || externalWallet.status === mojom.WalletStatus.kNotConnected) {
      return null
    }

    return externalWallet.status
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
      case 'zebpay': return 'zebpay'
      default: return null
    }
  }

  goToExternalWallet = () => {
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet || !externalWallet.accountUrl) {
      this.actions.getExternalWallet()
      return
    }

    window.open(externalWallet.accountUrl, '_self', 'noreferrer')
  }

  getAccountActivityLink = () => {
    const { externalWallet } = this.props.rewardsData

    if (!externalWallet) {
      return undefined
    }

    return externalWallet.status === mojom.WalletStatus.kConnected
         ? externalWallet.activityUrl : externalWallet.accountUrl
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
      converted: convertBalance(value, parameters.rate),
      link: key === 'ads' ? this.getAccountActivityLink() : undefined
    }
  }

  generateSummaryRows = (): SummaryItem[] => {
    return [
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
            const verified = isPublisherVerified(publisher.status)
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
                converted: convertBalance(publisher.weight, parameters.rate)
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
            converted: convertBalance(transaction.amount, parameters.rate)
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
                converted: convertBalance(contribution.amount, parameters.rate),
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

  isWalletProviderEnabled = (walletProvider: string) => {
    const { currentCountryCode, parameters } = this.props.rewardsData
    const regions = parameters.walletProviderRegions[walletProvider] || null
    return isExternalWalletProviderAllowed(currentCountryCode, regions)
  }

  generateExternalWalletProviderList = (walletProviders: string[]) => {
    return walletProviders.map((type) => ({
      type,
      name: lookupExternalWalletProviderName(type),
      enabled: this.isWalletProviderEnabled(type)
    }))
  }

  onExternalWalletAction = (action: ExternalWalletAction) => {
    switch (action) {
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
      isGrandfatheredUser,
      userType
    } = this.props.rewardsData
    const { modalReset, modalConnect } = ui

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
      <>
        {
          userType !== 'unconnected' &&
            <WalletCard
              userType={userType}
              balance={balance}
              isGrandfatheredUser={isGrandfatheredUser}
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
              onViewStatement={this.props.layout === 'wide' ? this.onModalActivityToggle : undefined}
            />
        }
        {
          modalReset
            ? <ModalReset
              onClose={this.onModalResetClose}
              onReset={this.onModalResetOnReset}
            />
            : null
        }
        {
          modalConnect
            ? <ConnectWalletModal
                currentCountryCode={this.props.rewardsData.currentCountryCode}
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
