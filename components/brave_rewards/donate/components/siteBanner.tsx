/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { SiteBanner } from 'brave-ui/features/rewards'

// Utils
import * as donateActions from '../actions/donate_actions'
import * as utils from '../utils'

interface Props extends RewardsDonate.ComponentProps {
}

interface State {
  currentAmount: number
}

class Banner extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      currentAmount: 0
    }
  }

  componentDidMount () {
    this.actions.getWalletProperties()
  }

  doNothing () {
    console.log('click')
  }

  get actions () {
    return this.props.actions
  }

  onClose = () => {
    this.actions.onCloseDialog()
  }

  generateAmounts = () => {
    const publisher = this.props.rewardsDonateData.publisher
    const walletInfo = this.props.rewardsDonateData.walletInfo
    let amounts = [1, 5, 10]

    if (publisher && publisher.amount) {
      amounts = publisher.amount
    }

    // { tokens: 1, converted: 0.3, selected: false }
    return amounts.map((value: number) => {
      return {
        tokens: value.toFixed(1),
        converted: utils.convertBalance(value.toString(), walletInfo.rates),
        selected: false
      }
    })
  }

  onAmountSelection = (tokens: number) => {
    this.setState({
      currentAmount: tokens
    })
  }

  onDonate = (amount: number, monthly: boolean) => {
    console.log('donate', amount, monthly)
  }

  generateSocialLinks = () => {
    const publisher = this.props.rewardsDonateData.publisher

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

  render () {
    const { publisher, walletInfo } = this.props.rewardsDonateData
    const { balance } = walletInfo

    let title = ''
    let background = ''
    let logo = ''
    let publisherKey = ''
    let description = ''

    if (publisher) {
      title = publisher.title
      // TODO re-enable when IsWebUIAllowedToMakeNetworkRequests crash is fixed
      // background = publisher.background
      // logo = publisher.logo
      publisherKey = publisher.publisherKey
      description = publisher.description
    }

    // TODO add current donation when donations are implemented
    // TODO we need to use title and not publisherKey for domain for media publishers

    return (
      <SiteBanner
        domain={publisherKey}
        title={title}
        recurringDonation={false}
        balance={balance.toString() || '0'}
        bgImage={background}
        logo={logo}
        donationAmounts={this.generateAmounts()}
        logoBgColor={''}
        onDonate={this.onDonate}
        onAmountSelection={this.onAmountSelection}
        currentAmount={this.state.currentAmount}
        onClose={this.onClose}
        social={this.generateSocialLinks()}
      >
        {description}
      </SiteBanner>
    )
  }
}

const mapStateToProps = (state: RewardsDonate.ApplicationState) => ({
  rewardsDonateData: state.rewardsDonateData
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(donateActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(Banner)
