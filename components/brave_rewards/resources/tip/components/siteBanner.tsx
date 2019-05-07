/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { SiteBanner } from 'brave-ui/features/rewards'
import { Provider } from 'brave-ui/features/rewards/profile'
import TweetBox from './tweetBox'

// Utils
import * as tipActions from '../actions/tip_actions'
import * as utils from '../utils'

interface Props extends RewardsTip.ComponentProps {
  publisher: RewardsTip.Publisher
  tweetMetaData?: RewardsTip.TweetMetaData
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
    this.actions.getRecurringTips()
    this.actions.getReconcileStamp()
  }

  get actions () {
    return this.props.actions
  }

  onClose = () => {
    this.actions.onCloseDialog()
  }

  generateAmounts = () => {
    const { walletInfo } = this.props.rewardsDonateData

    let amounts = [1, 5, 10]
    const amount = this.props.publisher.amounts
    if (amount && amount.length) {
      amounts = amount
    }

    return amounts.map((value: number) => {
      return {
        tokens: value.toFixed(1),
        converted: utils.convertBalance(value.toString(), walletInfo.rates),
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
    const { walletInfo } = this.props.rewardsDonateData
    const { balance } = walletInfo
    const publisher = this.props.publisher

    if (publisher.publisherKey && balance >= parseInt(amount, 10)) {
      this.actions.onTip(publisher.publisherKey, amount, recurring)
    } else {
      // TODO return error
    }
  }

  generateSocialLinks = () => {
    const publisher = this.props.publisher

    if (!publisher || !publisher.social) {
      return []
    }

    const socialLinks = publisher.social

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

  hasRecurringTip = (publisherKey?: string) => {
    const { recurringDonations } = this.props.rewardsDonateData

    if (!publisherKey || !recurringDonations) {
      return false
    }

    const recurringDonation = recurringDonations.find((donation: RewardsTip.RecurringTips) => {
      return donation.publisherKey === publisherKey
    })

    return !!recurringDonation
  }

  get addFundsLink () {
    return 'chrome://rewards/#add-funds'
  }

  render () {
    const { walletInfo } = this.props.rewardsDonateData
    const { balance } = walletInfo

    const publisher = this.props.publisher
    const verified = publisher.verified
    let logo = publisher.logo

    const internalFavicon = /^https:\/\/[a-z0-9-]+\.invalid(\/)?$/
    if (internalFavicon.test(publisher.logo)) {
      logo = `chrome://favicon/size/160@2x/${publisher.logo}`
    }

    let screenName
    if (this.props.tweetMetaData) {
      screenName = `@${this.props.tweetMetaData.screenName}`
    }

    if (!verified) {
      logo = ''
    }

    return (
      <SiteBanner
        domain={publisher.publisherKey}
        title={publisher.title}
        name={publisher.name}
        screenName={screenName}
        provider={publisher.provider as Provider}
        recurringDonation={this.hasRecurringTip(publisher.publisherKey)}
        balance={balance.toString() || '0'}
        bgImage={publisher.background}
        logo={logo}
        donationAmounts={this.generateAmounts()}
        logoBgColor={''}
        onDonate={this.onTip}
        onAmountSelection={this.onAmountSelection}
        currentAmount={this.state.currentAmount || '0'}
        onClose={this.onClose}
        social={this.generateSocialLinks()}
        showUnVerifiedNotice={!verified}
        learnMoreNotice={'https://brave.com/faq-rewards/#unclaimed-funds'}
        addFundsLink={this.addFundsLink}
      >
      {
        this.props.tweetMetaData
        ? <TweetBox tweetMetaData={this.props.tweetMetaData} />
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
