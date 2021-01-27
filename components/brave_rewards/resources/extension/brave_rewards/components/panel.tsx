/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { WalletAddIcon, BatColorIcon } from 'brave-ui/components/icons'
import { WalletWrapper, WalletSummary, WalletSummarySlider, WalletPanel } from '../../../ui/components'
import { Provider } from '../../../ui/components/profile'
import { NotificationType, WalletState } from '../../../ui/components/walletWrapper'
import { RewardsNotificationType } from '../constants/rewards_panel_types'
import { Type as AlertType } from '../../../ui/components/alert'
import { RewardsOptInModal, RewardsTourModal } from '../../../shared/components/onboarding'

// Utils
import * as rewardsPanelActions from '../actions/rewards_panel_actions'
import * as utils from '../utils'

import * as style from './panel.style'

import { getMessage } from '../background/api/locale_api'

interface Props extends RewardsExtension.ComponentProps {
  tabId: number,
  onlyAnonWallet: boolean
}

interface State {
  showSummary: boolean
  showRewardsTour: boolean
  firstTimeSetup: boolean
  publisherKey: string | null
  refreshingPublisher: boolean
  publisherRefreshed: boolean
  timerPassed: boolean
}

export class Panel extends React.Component<Props, State> {
  readonly defaultTipAmounts = [1, 5, 10]
  private delayTimer: ReturnType<typeof setTimeout>
  constructor (props: Props) {
    super(props)
    this.state = {
      showSummary: true,
      showRewardsTour: false,
      firstTimeSetup: false,
      publisherKey: null,
      refreshingPublisher: false,
      publisherRefreshed: false,
      timerPassed: false
    }
  }

  get actions () {
    return this.props.actions
  }

  componentDidMount () {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const newKey = publisher && publisher.publisherKey

    if (newKey) {
      this.setState({
        showSummary: false,
        publisherKey: newKey,
        refreshingPublisher: false,
        publisherRefreshed: false
      })
    }

    if (!this.props.rewardsPanelData.initializing) {
      this.startRewards()
    }
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const newKey = publisher && publisher.publisherKey

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

    if (prevProps.rewardsPanelData.initializing &&
        !this.props.rewardsPanelData.initializing) {
      this.startRewards()
    }
  }

  startRewards () {
    chrome.braveRewards.getACEnabled((enabled: boolean) => {
      this.props.actions.onEnabledAC(enabled)
    })

    chrome.braveRewards.shouldShowOnboarding((showOnboarding: boolean) => {
      this.props.actions.onShouldShowOnboarding(showOnboarding)
    })

    chrome.braveRewards.getPrefs((prefs) => {
      this.props.actions.onGetPrefs(prefs)
    })

    this.actions.fetchPromotions()

    chrome.braveRewards.getBalanceReport(new Date().getMonth() + 1, new Date().getFullYear(),
      (report: RewardsExtension.BalanceReport) => {
        this.actions.onBalanceReport(report)
      })

    chrome.braveRewards.getPendingContributionsTotal(((amount: number) => {
      this.actions.OnPendingContributionsTotal(amount)
    }))

    chrome.braveRewards.getRecurringTips((tips: RewardsExtension.RecurringTips) => {
      this.props.actions.onRecurringTips(tips)
    })

    chrome.braveRewards.getRewardsParameters((parameters: RewardsExtension.RewardsParameters) => {
      this.props.actions.onRewardsParameters(parameters)
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
    const publisherKey = publisher && publisher.publisherKey
    const excluded = publisher && publisher.excluded
    if (publisherKey && publisherKey.length > 0 && excluded !== undefined) {
      this.props.actions.includeInAutoContribution(publisherKey, !excluded)
    }
  }

  getPublisher = () => {
    let tabKey = this.props.tabId.toString()

    if (!tabKey) {
      return undefined
    }

    tabKey = `key_${tabKey}`

    return this.props.rewardsPanelData.publishers[tabKey]
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
      this.actions.deleteNotification(notification.id)
      const split = notification.id.split('_')
      const promoId = split[split.length - 1]
      if (promoId) {
        chrome.braveRewards.claimPromotion(promoId, (properties: RewardsExtension.Captcha) => {
          this.actions.onClaimPromotion(properties)
        })
      }
    }
  }

  onBackupWallet = (id: string) => {
    chrome.tabs.create({
      url: 'chrome://rewards#manage-wallet'
    })
    this.actions.deleteNotification(id)
  }

  onDeviceLimitReached = (id: string) => {
    chrome.tabs.create({
      url: 'https://support.brave.com/hc/en-us/articles/360056508071'
    })
    this.actions.deleteNotification(id)
  }

  onPromotionHide = (promotionId: string) => {
    this.actions.resetPromotion(promotionId)
  }

  onFinish = (promotionId: string) => {
    this.actions.deletePromotion(promotionId)
  }

  onSolution = (promotionId: string, x: number, y: number) => {
    let { promotions } = this.props.rewardsPanelData
    if (!promotionId || !promotions) {
      return
    }

    const currentPromotion = promotions.find((promotion: RewardsExtension.Promotion) => promotion.promotionId === promotionId)

    if (!currentPromotion) {
      return
    }

    if (!promotionId) {
      this.actions.promotionFinished(1)
      return
    }

    const data = JSON.stringify({
      captchaId: currentPromotion.captchaId,
      x: parseInt(x.toFixed(1), 10),
      y: parseInt(y.toFixed(1), 10)
    })

    chrome.braveRewards.attestPromotion(promotionId, data, (result: number, promotion?: RewardsExtension.Promotion) => {
      // if wrong position try again
      if (result === 6) {
        chrome.braveRewards.claimPromotion(promotionId, (properties: RewardsExtension.Captcha) => {
          this.actions.onClaimPromotion(properties)
        })
      }

      this.actions.promotionFinished(result, promotion)
    })
  }

  getWalletSummary = () => {
    const { parameters, balanceReport } = this.props.rewardsPanelData

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
    const { externalWallet, balance } = this.props.rewardsPanelData

    if (notificationId) {
      this.actions.deleteNotification(notificationId)
    }

    if (!externalWallet) {
      return
    }

    if (externalWallet.addUrl) {
      chrome.tabs.create({
        url: externalWallet.addUrl
      })
      return
    }

    utils.handleExternalWalletLink(balance, externalWallet)
  }

  showTipSiteDetail = (entryPoint: RewardsExtension.TipDialogEntryPoint) => {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const tabId = this.props.tabId

    if (!publisher || !publisher.publisherKey) {
      return
    }

    chrome.braveRewards.tipSite(tabId, publisher.publisherKey, entryPoint)
    window.close()
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
      case 'ads':
      case 'grant':
        clickEvent = this.onFetchCaptcha
        break
      case 'backupWallet':
        clickEvent = this.onBackupWallet.bind(this, id)
        break
      case 'insufficientFunds':
        clickEvent = this.onAddFunds.bind(this, id)
        break
      case 'deviceLimitReached':
        clickEvent = this.onDeviceLimitReached.bind(this, id)
        break
      default:
        clickEvent = undefined
        break
    }

    return clickEvent
  }

  getNotification = () => {
    const { notifications, currentNotification } = this.props.rewardsPanelData
    const { onlyAnonWallet } = this.props

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
          const currency = onlyAnonWallet ? getMessage('bap') : getMessage('bat')
          const contributionAmount = utils.handleContributionAmount(notification.args[3])
          text = getMessage('contributeNotificationSuccess', [contributionAmount, currency])
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
        type = 'ads'
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
          case 'wallet_new_verified': {
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
      case RewardsNotificationType.REWARDS_NOTIFICATION_DEVICE_LIMIT_REACHED:
        type = 'deviceLimitReached'
        text = getMessage('deviceLimitReachedNotification')
        break
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
    const newValue = parseFloat(event.target.value)
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()

    if (!publisher || !publisher.publisherKey) {
      return
    }

    const publisherKey = publisher.publisherKey

    if (newValue === 0) {
      this.actions.removeRecurringTip(publisherKey)
    } else {
      this.actions.saveRecurringTip(publisherKey, newValue)
    }
  }

  getContribution = (publisher?: RewardsExtension.Publisher) => {
    let defaultContribution = ''
    const { recurringTips } = this.props.rewardsPanelData

    if (!recurringTips ||
       (!publisher || !publisher.publisherKey)) {
      return defaultContribution
    }

    recurringTips.map((tip: any) => {
      if (tip && tip.publisherKey === publisher.publisherKey && tip.amount > 0) {
        defaultContribution = tip.amount.toFixed(3)
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

  resetPublisherStatus = (status: number) => {
    const verified = utils.isPublisherConnectedOrVerified(status)
    if (verified || this.state.timerPassed) {
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
    const publisherKey = publisher && publisher.publisherKey
    if (publisherKey) {
      chrome.braveRewards.refreshPublisher(publisherKey, (status: number, publisherKey: string) => {
        if (publisherKey) {
          this.actions.refreshPublisher(status, publisherKey)
        }
        this.resetPublisherStatus(status)
      })
    }
  }

  goToExternalWallet = () => {
    const { externalWallet } = this.props.rewardsPanelData

    if (!externalWallet || !externalWallet.accountUrl) {
      this.actions.getExternalWallet()
      return
    }

    window.open(externalWallet.accountUrl, '_blank')
  }

  onDisconnectClick = () => {
    chrome.braveRewards.disconnectWallet()
  }

  shouldShowConnectedMessage = () => {
    const { externalWallet, balance } = this.props.rewardsPanelData
    const { wallets } = balance
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const notVerified = publisher && utils.isPublisherNotVerified(publisher.status)
    const connected = publisher && utils.isPublisherConnected(publisher.status)
    const status = utils.getWalletStatus(externalWallet)

    if (notVerified) {
      return true
    }

    if (connected && (status === 'unverified' ||
      status === 'disconnected_unverified' ||
      status === 'disconnected_verified')) {
      return false
    }

    let nonUserFunds = 0

    if (wallets['anonymous']) {
      nonUserFunds += wallets['anonymous']
    }

    if (wallets['blinded']) {
      nonUserFunds += wallets['blinded']
    }

    const walletType = externalWallet ? externalWallet.type : ''
    switch (publisher ? publisher.status : 0) {
      case 1: // CONNECTED
        return nonUserFunds === 0
      case 2: // UPHOLD_VERIFIED
        return walletType !== 'uphold'
      case 3: // BITFLYER_VERIFIED
        return walletType !== 'bitflyer'
      default:
        return false
    }
  }

  getActions = () => {
    let actions = []

    if (!this.props.onlyAnonWallet) {
      actions.push({
        name: getMessage('addFunds'),
        action: this.onAddFunds,
        icon: <WalletAddIcon />,
        externalWallet: true
      })
    }

    return actions.concat([{
      name:  getMessage('rewardsSettings'),
      action: this.openRewardsPage,
      icon: <BatColorIcon />,
      externalWallet: false
    }])
  }

  getCurrentPromotion = (onlyAnonWallet: boolean) => {
    const { promotions } = this.props.rewardsPanelData

    if (!promotions) {
      return undefined
    }

    let currentPromotion = promotions.filter((promotion: RewardsExtension.Promotion) => {
      return promotion.captchaStatus !== null
    })

    if (currentPromotion.length === 0) {
      return undefined
    }

    return utils.getPromotion(currentPromotion[0], onlyAnonWallet)
  }

  showLoginMessage = () => {
    const { balance, externalWallet } = this.props.rewardsPanelData
    const walletStatus = utils.getWalletStatus(externalWallet)
    const walletType = externalWallet ? externalWallet.type : ''

    return (
      (!walletStatus || walletStatus === 'unverified') &&
      walletType === 'uphold' &&
      balance &&
      balance.total < 25
    )
  }

  showOnboarding () {
    const {
      showOnboarding,
      parameters,
      externalWallet,
      adsPerHour,
      autoContributeAmount
    } = this.props.rewardsPanelData

    // Hide AC options in rewards onboarding for bitFlyer-associated regions.
    let { autoContributeChoices } = parameters
    if (externalWallet && externalWallet.type === 'bitflyer') {
      autoContributeChoices = []
    }

    if (this.state.showRewardsTour) {
      const onDone = () => {
        this.setState({ showRewardsTour: false, firstTimeSetup: false })
      }

      const onClose = () => {
        onDone()
        if (showOnboarding) {
          this.actions.saveOnboardingResult('dismissed')
        }
      }

      const onAdsPerHourChanged = (adsPerHour: number) => {
        this.actions.updatePrefs({ adsPerHour })
      }

      const onAcAmountChanged = (autoContributeAmount: number) => {
        this.actions.updatePrefs({ autoContributeAmount })
      }

      return (
        <style.rewardsTourSpacer>
          <RewardsTourModal
            firstTimeSetup={this.state.firstTimeSetup}
            onlyAnonWallet={this.props.onlyAnonWallet}
            adsPerHour={adsPerHour}
            autoContributeAmount={autoContributeAmount}
            autoContributeAmountOptions={autoContributeChoices}
            onAdsPerHourChanged={onAdsPerHourChanged}
            onAutoContributeAmountChanged={onAcAmountChanged}
            onDone={onDone}
            onClose={onClose}
          />
        </style.rewardsTourSpacer>
      )
    }

    if (!showOnboarding) {
      return null
    }

    const onTakeTour = () => {
      this.setState({ showRewardsTour: true })
    }

    const onEnable = () => {
      this.actions.saveOnboardingResult('opted-in')

      // Ensure that we have the latest version of prefs when displaying
      // the rewards tour
      chrome.braveRewards.getPrefs((prefs) => {
        this.actions.onGetPrefs(prefs)
      })

      this.setState({ showRewardsTour: true, firstTimeSetup: true })
    }

    const onClose = () => {
      this.actions.saveOnboardingResult('dismissed')
    }

    return (
      <RewardsOptInModal
        onTakeTour={onTakeTour}
        onEnable={onEnable}
        onClose={onClose}
      />
    )
  }

  render () {
    const { pendingContributionTotal, enabledAC, externalWallet, balance, parameters } = this.props.rewardsPanelData
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const total = balance.total || 0
    const converted = utils.convertBalance(total, parameters.rate)
    const notification = this.getNotification()
    const notificationId = this.getNotificationProp('id', notification)
    const notificationType = this.getNotificationProp('type', notification)
    const notificationClick = this.getNotificationClickEvent(notificationType, notificationId)
    const defaultContribution = this.getContribution(publisher)
    const checkmark = publisher && utils.isPublisherConnectedOrVerified(publisher.status)
    const { onlyAnonWallet } = this.props

    const pendingTotal = parseFloat(
      (pendingContributionTotal || 0).toFixed(3))

    let faviconUrl
    if (publisher && publisher.url) {
      faviconUrl = `chrome://favicon/size/64@1x/${publisher.url}`
      if (publisher.favIconUrl && checkmark) {
        faviconUrl = `chrome://favicon/size/64@1x/${publisher.favIconUrl}`
      }
    }

    let currentPromotion = this.getCurrentPromotion(onlyAnonWallet)

    let walletStatus: WalletState | undefined = undefined
    let onVerifyClick = undefined
    if (!this.props.onlyAnonWallet) {
      walletStatus = utils.getWalletStatus(externalWallet)
      onVerifyClick = utils.handleExternalWalletLink.bind(this, balance, externalWallet)
    }

    return (
      <WalletWrapper
        id={'rewards-panel'}
        compact={true}
        contentPadding={false}
        gradientTop={this.gradientColor}
        balance={total.toFixed(3)}
        converted={utils.formatConverted(converted)}
        actions={this.getActions()}
        showCopy={false}
        showSecActions={false}
        grant={currentPromotion}
        onGrantHide={this.onPromotionHide}
        onNotificationClick={notificationClick}
        onSolution={this.onSolution}
        onFinish={this.onFinish}
        walletType={externalWallet ? externalWallet.type : undefined}
        walletState={walletStatus}
        walletProvider={utils.getWalletProviderName(externalWallet)}
        onVerifyClick={onVerifyClick}
        onDisconnectClick={this.onDisconnectClick}
        goToExternalWallet={this.goToExternalWallet}
        greetings={utils.getGreetings(externalWallet)}
        onlyAnonWallet={this.props.onlyAnonWallet}
        showLoginMessage={this.showLoginMessage()}
        {...notification}
      >
        <WalletSummarySlider
          id={'panel-slider'}
          onToggle={this.onSliderToggle}
        >
          {
            publisher && publisher.publisherKey
            ? <WalletPanel
              id={'wallet-panel'}
              platform={publisher.provider as Provider}
              publisherName={publisher.name}
              publisherImg={faviconUrl}
              monthlyAmount={defaultContribution}
              isVerified={checkmark}
              tipsEnabled={true}
              includeInAuto={!publisher.excluded}
              attentionScore={(publisher.percentage || 0).toString()}
              onToggleTips={this.doNothing}
              donationAction={this.showTipSiteDetail.bind(this, 'one-time')}
              onIncludeInAuto={this.switchAutoContribute}
              showUnVerified={this.shouldShowConnectedMessage()}
              acEnabled={enabledAC}
              moreLink={'https://brave.com/faq/#unclaimed-funds'}
              onRefreshPublisher={this.refreshPublisher}
              refreshingPublisher={this.state.refreshingPublisher}
              publisherRefreshed={this.state.publisherRefreshed}
              setMonthlyAction={this.showTipSiteDetail.bind(this, 'set-monthly')}
              cancelMonthlyAction={this.showTipSiteDetail.bind(this, 'clear-monthly')}
              onlyAnonWallet={onlyAnonWallet}
            />
            : null
          }
          <WalletSummary
            compact={true}
            reservedAmount={pendingTotal}
            onlyAnonWallet={this.props.onlyAnonWallet}
            reservedMoreLink={'https://brave.com/faq/#unclaimed-funds'}
            {...this.getWalletSummary()}
          />
        </WalletSummarySlider>
        {this.showOnboarding()}
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
