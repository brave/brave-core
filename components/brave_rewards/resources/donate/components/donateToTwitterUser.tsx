/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import Banner from './siteBanner'
import TransientDonationOverlay from './transientDonationOverlay'

// Utils
import * as rewardsActions from '../actions/donate_actions'
import { getLocale } from '../../../../common/locale'

interface Props extends RewardsDonate.ComponentProps {
  publisher: RewardsDonate.Publisher
  tweetMetaData: RewardsDonate.TweetMetaData
}

class DonateToTwitterUser extends React.Component<Props, {}> {

  get actions () {
    return this.props.actions
  }

  onTweet = () => {
    this.actions.onTweet(this.props.tweetMetaData)
    this.actions.onCloseDialog()
  }

  render () {
    const { finished, error } = this.props.rewardsDonateData

    const publisher = this.props.publisher
    const tweetMetaData = this.props.tweetMetaData
    publisher.title = getLocale('tweetTipTitle', { user: tweetMetaData.screenName })
    publisher.description = tweetMetaData.tweetText

    return (
      <>
        {
          !finished && !error
          ? <Banner publisher={publisher} />
          : null
        }
        {
          finished
          ? <TransientDonationOverlay
              publisher={publisher}
              timeout={0}
              onTweet={this.onTweet}
          />
          : null
        }
      </>
    )
  }
}

export const mapStateToProps = (state: RewardsDonate.ApplicationState) => ({
  rewardsDonateData: state.rewardsDonateData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(DonateToTwitterUser)
