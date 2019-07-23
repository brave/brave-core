/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { WalletAddIcon, BatColorIcon } from 'brave-ui/components/icons'
import { WalletWrapper, WalletSummary, WalletSummarySlider, WalletPanel, PanelVerify } from 'brave-ui/features/rewards'
import { Provider } from 'brave-ui/features/rewards/profile'
import { NotificationType, WalletState } from 'brave-ui/features/rewards/walletWrapper'
import { RewardsNotificationType } from '../constants/rewards_panel_types'
import { Type as AlertType } from 'brave-ui/features/rewards/alert'

// Utils
import * as rewardsPanelActions from '../actions/rewards_panel_actions'
import * as utils from '../utils'

import { getMessage } from '../background/api/locale_api'

interface Props extends RewardsExtension.ComponentProps {
  windowId: number
}

interface State {
  showSummary: boolean
  publisherKey: string | null
  refreshingPublisher: boolean
  publisherRefreshed: boolean
  timerPassed: boolean
  showVerifyOnBoarding: boolean
}

export class Panel extends React.Component<Props, State> {
  private defaultTipAmounts: number[]
  private delayTimer: ReturnType<typeof setTimeout>
  constructor (props: Props) {
    super(props)
    this.state = {
      showSummary: true,
      publisherKey: null,
      refreshingPublisher: false,
      publisherRefreshed: false,
      timerPassed: false,
      showVerifyOnBoarding: false
    }
    this.defaultTipAmounts = [1, 5, 10]
  }

  get actions () {
    return this.props.actions
  }

  componentDidMount () {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const newKey = publisher && publisher.publisher_key

    if (newKey) {
      this.setState({
        showSummary: false,
        publisherKey: newKey,
        refreshingPublisher: false,
        publisherRefreshed: false
      })
    }

    this.getBalance()
    this.actions.getGrants()
    this.actions.getWalletProperties()
    this.actions.getCurrentReport()

    chrome.braveRewards.getPendingContributionsTotal(((amount: number) => {
      this.actions.OnPendingContributionsTotal(amount)
    }))

    this.getExternalWallet()
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const newKey = publisher && publisher.publisher_key

    if (!prevState.publisherKey && newKey) {
      this.setState({
        showSummary: false,
        publisherKey: newKey
      })
    } else if (prevState.publisherKey && !newKey) {
      this.setState({
        showSummary: true,
        publisherKey: null
      })
    }

    if (!prevProps.rewardsPanelData.enabledMain && this.props.rewardsPanelData.enabledMain) {
      this.getBalance()
      this.actions.getGrants()
    }
  }

  getBalance () {
    chrome.braveRewards.fetchBalance((balance: RewardsExtension.Balance) => {
      this.actions.onBalance(balance)
    })
  }

  componentWillUnmount () {
    clearTimeout(this.delayTimer)
  }

  get gradientColor () {
    return this.state.showSummary ? '233,235,255' : '249,251,252'
  }

  doNothing () {
    console.log('doNothing click')
  }

  switchAutoContribute = () => {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const publisherKey = publisher && publisher.publisher_key
    const excluded = publisher && publisher.excluded
    if (publisherKey && publisherKey.length > 0 && excluded !== undefined) {
      this.props.actions.includeInAutoContribution(publisherKey, !excluded)
    }
  }

  getPublisher = () => {
    let windowId = this.props.windowId.toString()

    if (!windowId) {
      return undefined
    }

    windowId = `id_${windowId}`

    return this.props.rewardsPanelData.publishers[windowId]
  }

  onSliderToggle = () => {
    this.setState({
      showSummary: !this.state.showSummary
    })
  }

  onFetchCaptcha = () => {
    let notification: any = this.getNotification()

    if (!notification || (notification && notification.alert)) {
      return
    }

    notification = notification && notification.notification

    if (notification && notification.id) {
      const split = notification.id.split('_')
      const promoId = split[split.length - 1]
      if (promoId) {
        this.actions.getGrantCaptcha(promoId)
      }
    }
  }

  onBackupWallet = (id: string) => {
    chrome.tabs.create({
      url: 'chrome://rewards#backup-restore'
    })
    this.actions.deleteNotification(id)
  }

  onGrantHide = () => {
    this.actions.onResetGrant()
  }

  onFinish = () => {
    this.actions.onDeleteGrant()
  }

  onSolution = (x: number, y: number) => {
    this.actions.solveGrantCaptcha(x, y)
  }

  getWalletSummary = () => {
    const { balance, report } = this.props.rewardsPanelData
    const { rates } = balance

    let props = {}

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

  openRewardsPage (notificationId?: string) {
    chrome.tabs.create({
      url: 'brave://rewards'
    })

    if (notificationId) {
      this.onCloseNotification(notificationId)
    }
  }

  onAddFunds = (notificationId?: string) => {
    const status = this.getWalletStatus()
    const { externalWallet } = this.props.rewardsPanelData

    if (notificationId) {
      this.actions.deleteNotification(notificationId)
    }

    if (!externalWallet) {
      return
    }

    // WalletStatus::CONNECTED
    if (status === 'verified' || externalWallet.status === 1) {
      if (externalWallet.addUrl) {
        chrome.tabs.create({
          url: externalWallet.addUrl
        })
      }
      return
    }

    if (externalWallet.verifyUrl) {
      this.handleUpholdLink(externalWallet.verifyUrl)
      return
    }
  }

  showTipSiteDetail = () => {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    // TODO: why do we store windowId instead of active tab id in state?
    chrome.tabs.query({
      active: true,
      windowId: chrome.windows.WINDOW_ID_CURRENT
    }, (tabs) => {
      if (!tabs || !tabs.length) {
        return
      }
      const tabId = tabs[0].id
      if (tabId === undefined || !publisher || !publisher.publisher_key) {
        return
      }

      chrome.braveRewards.tipSite(tabId, publisher.publisher_key)
      window.close()
    })
  }

  onCloseNotification = (id: string) => {
    this.actions.deleteNotification(id)
  }

  getNotificationProp = (key: string, notification: any) => {
    if (!notification ||
        !notification.notification ||
        !notification.notification[key]) {
      return null
    }

    return notification.notification[key]
  }

  getNotificationClickEvent = (type?: string, id?: string) => {
    if (!type) {
      return undefined
    }

    let clickEvent

    switch (type) {
      case 'grant':
        clickEvent = this.onFetchCaptcha
        break
      case 'backupWallet':
        clickEvent = this.onBackupWallet.bind(this, id)
        break
      case 'ads-launch':
        clickEvent = this.openRewardsPage.bind(this, id)
        break
      case 'insufficientFunds':
        clickEvent = this.onAddFunds.bind(this, id)
        break
      default:
        clickEvent = undefined
        break
    }

    return clickEvent
  }

  getNotification = () => {
    const { notifications, currentNotification } = this.props.rewardsPanelData

    if (
      currentNotification === undefined ||
      !notifications ||
      (currentNotification && !notifications[currentNotification])
    ) {
      return undefined
    }

    const notification: RewardsExtension.Notification = notifications[currentNotification]

    let type: NotificationType = ''
    let text
    let isAlert = ''

    switch (notification.type) {
      case RewardsNotificationType.REWARDS_NOTIFICATION_AUTO_CONTRIBUTE: {
        if (!notification.args ||
          !Array.isArray(notification.args) ||
          notification.args.length < 3) {
          break
        }

        const result = notification.args[1]

        // Results
        // 0 - success
        // 1 - general error
        // 15 - not enough funds
        // 16 - error while tipping

        if (result === '0') {
          const fixed = utils.convertProbiToFixed(notification.args[3])
          text = getMessage('contributeNotificationSuccess', [fixed])
        } else if (result === '15') {
          text = getMessage('contributeNotificationNotEnoughFunds')
          isAlert = 'warning'
        } else if (result === '16') {
          text = getMessage('contributeNotificationTipError')
          isAlert = 'error'
        } else {
          text = getMessage('contributeNotificationError')
          isAlert = 'error'
        }
        type = 'contribute'
        break
      }
      case RewardsNotificationType.REWARDS_NOTIFICATION_GRANT:
        type = 'grant'
        text = getMessage('grantNotification')
        break
      case RewardsNotificationType.REWARDS_NOTIFICATION_GRANT_ADS:
        type = 'grant'
        text = getMessage('earningsClaimDefault')
        break
      case RewardsNotificationType.REWARDS_NOTIFICATION_BACKUP_WALLET:
        type = 'backupWallet'
        text = getMessage('backupWalletNotification')
        break
      case RewardsNotificationType.REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS:
        type = 'insufficientFunds'
        text = getMessage('insufficientFundsNotification')
        break
      case RewardsNotificationType.REWARDS_NOTIFICATION_TIPS_PROCESSED:
        type = 'tipsProcessed'
        text = getMessage('tipsProcessedNotification')
        break
      case RewardsNotificationType.REWARDS_NOTIFICATION_ADS_LAUNCH:
        type = 'ads-launch'
        text = getMessage('braveAdsLaunchMsg')
        break
      case RewardsNotificationType.REWARDS_NOTIFICATION_VERIFIED_PUBLISHER: {
        let name = ''
        if (notification.args &&
            Array.isArray(notification.args) &&
            notification.args.length > 0) {
          name = notification.args[0]
        }

        type = 'pendingContribution'
        text = getMessage('verifiedPublisherNotification', [name])
        break
      }
      case RewardsNotificationType.REWARDS_NOTIFICATION_PENDING_NOT_ENOUGH_FUNDS:
        type = 'insufficientFunds'
        text = getMessage('pendingNotEnoughFundsNotification')
        break
      case RewardsNotificationType.REWARDS_NOTIFICATION_GENERAL_LEDGER: {
        const args = notification.args
        if (!args ||
          !Array.isArray(args) ||
          args.length === 0) {
          break
        }

        const type = args[0]

        switch (type) {
          case 'wallet_verified': {
            text = (
              <>
                <div><b>{getMessage('walletVerifiedNotification')}</b></div>
                {getMessage('walletVerifiedTextNotification', [args[1]])}
              </>
            )
            isAlert = 'success'
            break
          }
          case 'wallet_disconnected': {
            text = (
              <>
                <div><b>{getMessage('walletDisconnectedNotification')}</b></div>
                {getMessage('walletDisconnectedTextNotification')}
              </>
            )
            isAlert = 'error'
            break
          }
          default:
            break
        }

        break
      }
      default:
        type = ''
        break
    }

    if (isAlert) {
      return {
        alert: {
          node: text,
          type: isAlert as AlertType,
          onAlertClose: this.onCloseNotification.bind(this, notification.id)
        }
      }
    }

    return {
      notification: {
        id: notification.id,
        date: new Date(notification.timestamp * 1000).toLocaleDateString(),
        type: type,
        text: text,
        onCloseNotification: this.onCloseNotification
      }
    }
  }

  onContributionAmountChange = (event: React.ChangeEvent<HTMLSelectElement>) => {
    const newValue = parseInt(event.target.value, 10)
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()

    if (!publisher || !publisher.publisher_key) {
      return
    }

    const publisherKey = publisher.publisher_key

    if (newValue === 0) {
      this.actions.removeRecurringTip(publisherKey)
    } else {
      this.actions.saveRecurringTip(publisherKey, newValue)
    }
  }

  generateAmounts = (publisher?: RewardsExtension.Publisher) => {
    const { tipAmounts } = this.props.rewardsPanelData
    const { rates } = this.props.rewardsPanelData.balance

    const publisherKey = publisher && publisher.publisher_key
    const initialAmounts = (
      !publisherKey ||
      !tipAmounts ||
      !tipAmounts[publisherKey] ||
      tipAmounts[publisherKey].length === 0
    ) ? this.defaultTipAmounts : tipAmounts[publisherKey]

    const amounts = [0, ...initialAmounts]

    return amounts.map((value: number) => {
      return {
        tokens: value.toFixed(1),
        converted: utils.convertBalance(value.toString(), rates),
        selected: false
      }
    })
  }

  getContribution = (publisher?: RewardsExtension.Publisher) => {
    let defaultContribution = '0.0'
    const { recurringTips } = this.props.rewardsPanelData

    if (!recurringTips ||
       (!publisher || !publisher.publisher_key)) {
      return defaultContribution
    }

    recurringTips.map((tip: any) => {
      if (tip.publisherKey === publisher.publisher_key) {
        defaultContribution = tip.amount.toFixed(1)
      }
    })

    return defaultContribution
  }

  initiateDelayCounter = () => {
    clearTimeout(this.delayTimer)
    this.delayTimer = setTimeout(() => {
      this.setState({
        timerPassed: true
      })
    }, 2000)
  }

  resetPublisherStatus = (wasVerified: boolean) => {
    if (wasVerified || this.state.timerPassed) {
      this.setState({
        timerPassed: false,
        refreshingPublisher: false,
        publisherRefreshed: true
      })
    } else {
      setTimeout(this.resetPublisherStatus, 250)
    }
  }

  refreshPublisher = () => {
    this.setState({
      refreshingPublisher: true,
      timerPassed: false
    })
    this.initiateDelayCounter()
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const publisherKey = publisher && publisher.publisher_key
    if (publisherKey) {
      chrome.braveRewards.refreshPublisher(publisherKey, (verified: boolean, publisherKey: string) => {
        if (publisherKey) {
          this.actions.refreshPublisher(verified, publisherKey)
        }
        this.resetPublisherStatus(verified)
      })
    }
  }

  getExternalWallet = (open: boolean = false) => {
    chrome.braveRewards.getExternalWallet('uphold', (result: number, wallet: RewardsExtension.ExternalWallet) => {
      // EXPIRED TOKEN
      if (result === 24) {
        this.getExternalWallet(open)
        return
      }

      this.actions.onExternalWallet(wallet)

      if (open && wallet.verifyUrl) {
        this.handleUpholdLink(wallet.verifyUrl)
      }
    })
  }

  handleUpholdLink = (link: string) => {
    const { ui } = this.props.rewardsPanelData
    if (!this.state.showVerifyOnBoarding && (!ui || !ui.onBoardingDisplayed)) {
      this.setState({
        showVerifyOnBoarding: true
      })
      return
    }

    this.setState({
      showVerifyOnBoarding: false
    })

    this.actions.onOnBoardingDisplayed()

    chrome.tabs.create({
      url: link
    })
  }

  onVerifyClick = () => {
    const { externalWallet } = this.props.rewardsPanelData

    if (!externalWallet || externalWallet.verifyUrl) {
      this.getExternalWallet(true)
      return
    }

    this.handleUpholdLink(externalWallet.verifyUrl)
  }

  toggleVerifyPanel = () => {
    this.setState({
      showVerifyOnBoarding: !this.state.showVerifyOnBoarding
    })
  }

  getWalletStatus = (): WalletState => {
    const { externalWallet } = this.props.rewardsPanelData

    if (!externalWallet) {
      return 'unverified'
    }

    switch (externalWallet.status) {
      // ledger::WalletStatus::VERIFIED
      case 2:
        return 'verified'
      // ledger::WalletStatus::DISCONNECTED_NOT_VERIFIED
      case 3:
        return 'disconnected_unverified'
      // ledger::WalletStatus::DISCONNECTED_VERIFIED
      case 4:
        return 'disconnected_verified'
      default:
        return 'unverified'
    }
  }

  goToUphold = () => {
    const { externalWallet } = this.props.rewardsPanelData

    if (!externalWallet || !externalWallet.accountUrl) {
      this.actions.getExternalWallet('uphold')
      return
    }

    window.open(externalWallet.accountUrl, '_blank')
  }

  getUserName = () => {
    const { externalWallet } = this.props.rewardsPanelData
    if (!externalWallet) {
      return ''
    }

    return externalWallet.userName
  }

  onDisconnectClick = () => {
    chrome.braveRewards.disconnectWallet('uphold')
  }

  render () {
    const { pendingContributionTotal, enabledAC } = this.props.rewardsPanelData
    const { total, rates } = this.props.rewardsPanelData.balance
    const { grants } = this.props.rewardsPanelData.walletProperties
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const converted = utils.convertBalance(total.toString(), rates)
    const notification = this.getNotification()
    const notificationId = this.getNotificationProp('id', notification)
    const notificationType = this.getNotificationProp('type', notification)
    const notificationClick = this.getNotificationClickEvent(notificationType, notificationId)
    let { currentGrant } = this.props.rewardsPanelData
    const defaultContribution = this.getContribution(publisher)
    const tipAmounts = defaultContribution !== '0.0'
      ? this.generateAmounts(publisher)
      : undefined

    if (notification &&
        notification.notification &&
        notificationType === 'ads-launch') {
      delete notification.notification['date']
    }

    const pendingTotal = parseFloat(
      (pendingContributionTotal || 0).toFixed(1))

    let faviconUrl
    if (publisher && publisher.url) {
      faviconUrl = `chrome://favicon/size/48@2x/${publisher.url}`
      if (publisher.favicon_url && publisher.verified) {
        faviconUrl = `chrome://favicon/size/48@2x/${publisher.favicon_url}`
      }
    }

    currentGrant = utils.getGrant(currentGrant)

    return (
      <WalletWrapper
        compact={true}
        contentPadding={false}
        gradientTop={this.gradientColor}
        balance={total.toFixed(1)}
        converted={utils.formatConverted(converted)}
        actions={[
          {
            name: getMessage('addFunds'),
            action: this.onAddFunds,
            icon: <WalletAddIcon />
          },
          {
            name:  getMessage('rewardsSettings'),
            action: this.openRewardsPage,
            icon: <BatColorIcon />
          }
        ]}
        showCopy={false}
        showSecActions={false}
        grant={currentGrant}
        onGrantHide={this.onGrantHide}
        onNotificationClick={notificationClick}
        onSolution={this.onSolution}
        onFinish={this.onFinish}
        convertProbiToFixed={utils.convertProbiToFixed}
        grants={utils.getGrants(grants)}
        walletState={this.getWalletStatus()}
        onVerifyClick={this.onVerifyClick}
        onDisconnectClick={this.onDisconnectClick}
        goToUphold={this.goToUphold}
        userName={this.getUserName()}
        {...notification}
      >
        {
          this.state.showVerifyOnBoarding
          ? <PanelVerify
            compact={true}
            onVerifyClick={this.onVerifyClick}
            onClose={this.toggleVerifyPanel}
          />
          : null
        }
        <WalletSummarySlider
          id={'panel-slider'}
          onToggle={this.onSliderToggle}
        >
          {
            publisher && publisher.publisher_key
            ? <WalletPanel
              id={'wallet-panel'}
              platform={publisher.provider as Provider}
              publisherName={publisher.name}
              publisherImg={faviconUrl}
              monthlyAmount={defaultContribution}
              isVerified={publisher.verified}
              tipsEnabled={true}
              includeInAuto={!publisher.excluded}
              attentionScore={(publisher.percentage || 0).toString()}
              onToggleTips={this.doNothing}
              donationAction={this.showTipSiteDetail}
              onAmountChange={this.onContributionAmountChange}
              onIncludeInAuto={this.switchAutoContribute}
              showUnVerified={!publisher.verified}
              acEnabled={enabledAC}
              donationAmounts={tipAmounts}
              moreLink={'https://brave.com/faq/#unclaimed-funds'}
              onRefreshPublisher={this.refreshPublisher}
              refreshingPublisher={this.state.refreshingPublisher}
              publisherRefreshed={this.state.publisherRefreshed}
            />
            : null
          }
          <WalletSummary
            compact={true}
            reservedAmount={pendingTotal}
            reservedMoreLink={'https://brave.com/faq/#unclaimed-funds'}
            {...this.getWalletSummary()}
          />
        </WalletSummarySlider>
      </WalletWrapper>
    )
  }
}

export const mapStateToProps = (state: RewardsExtension.ApplicationState) => ({
  rewardsPanelData: state.rewardsPanelData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsPanelActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(Panel)
