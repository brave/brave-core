/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Utils
import * as donateActions from '../actions/donate_actions'

// Assets
const twitterImg = require('../../img/twitter.svg')

interface Props extends RewardsDonate.ComponentProps {
  tweetMetaData: RewardsDonate.TweetMetaData
}

class TweetBox extends React.Component<Props, {}> {
  get actions () {
    return this.props.actions
  }

  formatDate = (date: Date) => {
    const dateOptions = { month: 'short', day: 'numeric' }
    if (new Date().getFullYear() !== date.getFullYear()) {
      dateOptions['year'] = 'numeric'
    }
    return date.toLocaleString(navigator.language, dateOptions)
  }

  render () {
    const tweetDate = new Date(this.props.tweetMetaData.tweetTimestamp * 1000)
    return (
      <div style={{ border: '1px solid #d3d3d3', borderRadius: '5px', margin: '15px 0 0 0', padding: '15px', textOverflow: 'ellipsis', whiteSpace: 'pre-wrap', overflow: 'hidden' }}>
        <div>
          <img style={{ width: '24px', height: '24px', display: 'inline', verticalAlign: 'middle' }} src={twitterImg} />
          <div style={{ color: '#a9a9a9', fontSize: '12px', display: 'inline', verticalAlign: 'middle' }}>
            {this.formatDate(tweetDate)}
          </div>
        </div>
        <p style={{ fontSize: '14px' }}>
          {this.props.tweetMetaData.tweetText}
        </p>
      </div>
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
)(TweetBox)
