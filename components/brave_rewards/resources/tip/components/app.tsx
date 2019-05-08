/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import TipSite from './tipSite'
import TipTwitterUser from './tipTwitterUser'

// Utils
import * as rewardsActions from '../actions/tip_actions'

interface TipDialogArgs {
  publisherKey: string
  tweetMetaData?: RewardsTip.TweetMetaData
}

interface Props extends RewardsTip.ComponentProps {
  dialogArgs: TipDialogArgs
}

export class App extends React.Component<Props, {}> {

  get actions () {
    return this.props.actions
  }

  render () {
    const { publishers } = this.props.rewardsDonateData

    if (!publishers) {
      return null
    }

    const publisherKey = this.props.dialogArgs.publisherKey
    const publisher = publishers[publisherKey]

    if (!publisher) {
      return null
    }

    let tip
    const tweetMetaData = this.props.dialogArgs.tweetMetaData
    if (tweetMetaData) {
      tip = (
        <TipTwitterUser
          publisher={publisher}
          tweetMetaData={tweetMetaData}
        />
      )
    } else {
      tip = (
        <TipSite
          publisher={publisher}
        />
      )
    }

    return (
      <div>
        {tip}
      </div>
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
)(App)
