/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import Banner from './siteBanner'

// Utils
import * as rewardsActions from '../actions/tip_actions'
import { getLocale } from '../../../../common/locale'

interface Props extends RewardsTip.ComponentProps {
  url: string
  monthly: boolean
  monthlyDate?: string
  amount?: string
  publisher: RewardsTip.Publisher
  mediaMetaData: RewardsTip.MediaMetaData
}

class TipMediaUser extends React.Component<Props, {}> {

  get actions () {
    return this.props.actions
  }

  onTweet = () => {
    const mediaMetaData = this.props.mediaMetaData
    if (!mediaMetaData) {
      return
    }

    if (mediaMetaData.mediaType === 'twitter') {
      this.actions.onTweet(
        `@${mediaMetaData.screenName}`,
        mediaMetaData.tweetId)
    } else if (mediaMetaData.mediaType === 'reddit') {
      let name = this.props.publisher.name
      this.actions.onTweet(name, '')
    } else if (mediaMetaData.mediaType === 'github') {
      let name = this.props.publisher.name
      this.actions.onTweet(name, '')
    }
    this.actions.onCloseDialog()
  }

  render () {
    const { finished, error, onlyAnonWallet } = this.props.rewardsDonateData
    const {
      monthly,
      monthlyDate,
      mediaMetaData,
      publisher,
      amount
    } = this.props

    if (!mediaMetaData) {
      return
    }

    if (mediaMetaData.mediaType === 'twitter') {
      const key =
        mediaMetaData.tweetText &&
        mediaMetaData.tweetText.length > 0
        ? 'tweetTipTitle'
        : 'tweetTipTitleEmpty'
      publisher.title = getLocale(key, { user: mediaMetaData.screenName })
    } else if (mediaMetaData.mediaType === 'reddit') {
      const key =
        mediaMetaData.postText &&
        mediaMetaData.postText.length > 0
          ? 'redditTipTitle'
          : 'redditTipTitleEmpty'
      publisher.title = getLocale(key, { user: 'u/' + mediaMetaData.userName })
    } else if (mediaMetaData.mediaType === 'github') {
      const key = mediaMetaData.userName ? 'githubTipTitle' : 'githubTipTitleEmpty'
      publisher.title = getLocale(key, { user: '@' + mediaMetaData.userName })
    }

    return (
      <>
        {
          !error
          ? <Banner
              monthly={monthly}
              publisher={publisher}
              tipComplete={finished}
              onTweet={this.onTweet}
              mediaMetaData={mediaMetaData}
              onlyAnonWallet={onlyAnonWallet}
              monthlyDate={monthlyDate}
              amount={amount}
          />
          : null
        }
      </>
    )
  }
}

export const mapStateToProps = (state: RewardsTip.ApplicationState) => ({
  rewardsDonateData: state.rewardsDonateData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(TipMediaUser)
