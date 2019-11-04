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
  publisher: RewardsTip.Publisher
  tipComplete?: boolean
  onTweet: () => void
  mediaMetaData?: RewardsTip.MediaMetaData
}

interface State {
  currentAmount: string
}

class Banner extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      currentAmount: '0'
    }
  }

  componentDidMount () {
    this.actions.getWalletProperties()
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
    const { balance } = this.props.rewardsDonateData

    let amounts = [1, 5, 10]
    const amount = this.props.publisher.amounts
    if (amount && amount.length) {
      amounts = amount
    }

    return amounts.map((value: number) => {
      return {
        tokens: value.toFixed(1),
        converted: utils.convertBalance(value.toString(), balance.rates),
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

    if (publisher.publisherKey && total >= parseInt(amount, 10)) {
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
      return `@${mediaMetaData.screenName}`
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
        !mediaMetaData.tweetText ||
        mediaMetaData.tweetText.length === 0) {
      return null
    }

    return (
      <MediaBox
        mediaType={'twitter'}
        mediaText={mediaMetaData.tweetText}
        mediaTimestamp={mediaMetaData.tweetTimestamp}
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

    const hasAnonBalance = wallets['anonymous'] && wallets['anonymous'] > 0

    return connected && !hasAnonBalance
  }

  render () {
    const { balance } = this.props.rewardsDonateData
    const { total } = balance

    const mediaMetaData = this.props.mediaMetaData
    const publisher = this.props.publisher
    const checkmark = utils.isPublisherConnectedOrVerified(publisher.status)
    const bannerType = this.props.monthly ? 'monthly' : 'one-time'
    let logo = publisher.logo
    const currentBalance = (total && total.toFixed(1)) || '0.0'

    const internalFavicon = /^https:\/\/[a-z0-9-]+\.invalid(\/)?$/
    if (internalFavicon.test(publisher.logo)) {
      logo = `chrome://favicon/size/160@2x/${publisher.logo}`
    }

    if (!checkmark) {
      logo = ''
    }
    return (
      <SiteBanner
        type={bannerType}
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
