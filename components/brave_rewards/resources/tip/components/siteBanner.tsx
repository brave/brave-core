/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Provider } from '../../ui/components/profile'
import { SiteBanner, MediaBox } from '../../ui/components'

// Utils
import * as tipActions from '../actions/tip_actions'
import * as utils from '../utils'

interface Props extends RewardsTip.ComponentProps {
  monthly: boolean
  monthlyDate?: string
  amount?: string
  onlyAnonWallet?: boolean
  publisher: RewardsTip.Publisher
  tipComplete?: boolean
  onTweet: () => void
  mediaMetaData?: RewardsTip.MediaMetaData
}

interface State {
  currentAmount: string
}

class Banner extends React.Component<Props, State> {
  readonly defaultTipAmounts = [1, 5, 10]
  constructor (props: Props) {
    super(props)
    this.state = {
      currentAmount: '0'
    }
  }

  componentDidMount () {
    this.actions.getRewardsParameters()
    this.actions.getBalance()
    this.actions.getRecurringTips()
    this.actions.getReconcileStamp()
    this.actions.getExternalWallet()
  }

  get actions () {
    return this.props.actions
  }

  onClose = () => {
    this.actions.onCloseDialog()
  }

  generateAmounts = () => {
    const { monthly } = this.props
    const { parameters } = this.props.rewardsDonateData

    const publisherAmounts = this.props.publisher.amounts

    // Prefer the publisher amounts, then the wallet's defaults. Fall back to defaultTipAmounts.
    let amounts = this.defaultTipAmounts
    if (publisherAmounts && publisherAmounts.length) {
      amounts = publisherAmounts
    } else if (parameters) {
      const walletAmounts = monthly
        ? parameters.monthlyTipChoices
        : parameters.tipChoices

      if (walletAmounts.length) {
        amounts = walletAmounts
      }
    }

    return amounts.map((value: number) => {
      return {
        tokens: value.toFixed(3),
        converted: utils.convertBalance(value.toString(), parameters.rate),
        selected: false
      }
    })
  }

  onAmountSelection = (tokens: string) => {
    this.setState({
      currentAmount: tokens
    })
  }

  onTip = (amount: string, recurring: boolean) => {
    const { balance } = this.props.rewardsDonateData
    const { total } = balance
    const publisher = this.props.publisher

    if (publisher.publisherKey && total >= parseFloat(amount)) {
      this.actions.onTip(publisher.publisherKey, amount, recurring)
    } else {
      // TODO return error
    }
  }

  generateSocialLinks = () => {
    const publisher = this.props.publisher

    if (!publisher || !publisher.links) {
      return []
    }

    const socialLinks = publisher.links

    // TODO export social type from site banner component
    let result = [] as any

    Object.keys(socialLinks).forEach(key => {
      if (socialLinks[key] && socialLinks[key].length > 0) {
        result.push({
          type: key,
          url: socialLinks[key]
        })
      }
    })

    return result
  }

  getScreenName = (mediaMetaData?: RewardsTip.MediaMetaData) => {
    if (!mediaMetaData) {
      return ''
    }
    if (mediaMetaData.mediaType === 'twitter') {
      return `@${mediaMetaData.publisherName}`
    } else if (mediaMetaData.mediaType === 'reddit') {
      return `u/${mediaMetaData.userName}`
    } else if (mediaMetaData.mediaType === 'github') {
      return `@${mediaMetaData.userName}`
    }
    return ''
  }

  get addFundsLink () {
    return 'chrome://rewards/#add-funds'
  }

  getTweetText () {
    const mediaMetaData = this.props.mediaMetaData
    if (!mediaMetaData) {
      return
    }

    if (mediaMetaData.mediaType !== 'twitter' ||
        !mediaMetaData.postText ||
        mediaMetaData.postText.length === 0) {
      return null
    }

    return (
      <MediaBox
        mediaType={'twitter'}
        mediaText={mediaMetaData.postText}
        mediaTimestamp={Date.parse(mediaMetaData.postTimestamp) / 1000}
      />)
  }

  getRedditText () {
    const mediaMetaData = this.props.mediaMetaData
    if (!mediaMetaData) {
      return
    }

    if (mediaMetaData.mediaType !== 'reddit' ||
      !mediaMetaData.postText ||
      mediaMetaData.postText.length === 0) {
      return null
    }

    return (
      <MediaBox
        mediaType={'reddit'}
        mediaText={mediaMetaData.postText}
        mediaTimestamp={0}
        mediaTimetext={mediaMetaData.postRelDate}
      />)
  }

  getNextContribution = () => {
    const { reconcileStamp } = this.props.rewardsDonateData

    if (!reconcileStamp) {
      return undefined
    }

    const fmtArgs = {
      day: '2-digit',
      month: '2-digit',
      year: 'numeric'
    }

    return new Intl.DateTimeFormat('default', fmtArgs).format(reconcileStamp * 1000)
  }

  shouldShowConnectedMessage = () => {
    const publisher = this.props.publisher
    const { externalWallet, balance } = this.props.rewardsDonateData
    const { wallets } = balance
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

    return connected && nonUserFunds === 0
  }

  render () {
    const { balance } = this.props.rewardsDonateData
    const { total } = balance
    const { onlyAnonWallet, publisher, mediaMetaData, monthlyDate, amount } = this.props

    const checkmark = utils.isPublisherConnectedOrVerified(publisher.status)
    const bannerType = this.props.monthly ? 'monthly' : 'one-time'
    let logo = publisher.logo
    const currentBalance = (total && total.toFixed(3)) || '0.000'

    const internalFavicon = /^https:\/\/[a-z0-9-]+\.invalid(\/)?$/
    if (internalFavicon.test(publisher.logo)) {
      logo = `chrome://favicon/size/160@1x/${publisher.logo}`
    }

    if (!checkmark) {
      logo = ''
    }
    return (
      <SiteBanner
        type={bannerType}
        onlyAnonWallet={onlyAnonWallet}
        domain={publisher.publisherKey}
        title={publisher.title}
        name={publisher.name}
        screenName={this.getScreenName(mediaMetaData)}
        provider={publisher.provider as Provider}
        balance={currentBalance}
        bgImage={publisher.background}
        logo={logo}
        donationAmounts={this.generateAmounts()}
        logoBgColor={''}
        onDonate={this.onTip}
        onAmountSelection={this.onAmountSelection}
        currentAmount={this.state.currentAmount || '0'}
        onClose={this.onClose}
        social={this.generateSocialLinks()}
        showUnVerifiedNotice={this.shouldShowConnectedMessage()}
        isVerified={checkmark}
        learnMoreNotice={'https://brave.com/faq/#unclaimed-funds'}
        addFundsLink={this.addFundsLink}
        tipComplete={this.props.tipComplete}
        onTweet={this.props.onTweet}
        nextContribution={this.getNextContribution()}
        monthlyDate={monthlyDate}
        amount={amount}
      >
      {
        mediaMetaData
          ? mediaMetaData.mediaType === 'twitter'
          ? this.getTweetText()
          : mediaMetaData.mediaType === 'reddit'
          ? this.getRedditText()
          : publisher.description
        : publisher.description
      }
      </SiteBanner>
    )
  }
}

const mapStateToProps = (state: RewardsTip.ApplicationState) => ({
  rewardsDonateData: state.rewardsDonateData
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(tipActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(Banner)
