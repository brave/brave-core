/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import Banner from './siteBanner'
import TransientTipOverlay from './transientTipOverlay'

// Utils
import * as rewardsActions from '../actions/tip_actions'

interface Props extends RewardsTip.ComponentProps {
  url: string
  publisher: RewardsTip.Publisher
}

class TipSite extends React.Component<Props, {}> {
  get actions () {
    return this.props.actions
  }

  onTweet = () => {
    let name = this.props.publisher.name
    if (this.props.publisher.provider === 'twitter') {
      const url = this.props.url
      if (url && url.length > 0) {
        name = `@${url.replace(/^.*\/(.*)$/, '$1')}`
      }
    }

    this.actions.onTweet(name, '')
    this.actions.onCloseDialog()
  }

  render () {
    const { finished, error, onlyAnonWallet } = this.props.rewardsDonateData

    return (
      <>
        {
          !finished && !error
          ? <Banner onlyAnonWallet={onlyAnonWallet} publisher={this.props.publisher} />
          : null
        }
        {
          finished
          ? <TransientTipOverlay
              publisher={this.props.publisher}
              onTweet={this.onTweet}
              timeout={3000}
              onlyAnonWallet={onlyAnonWallet}
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
)(TipSite)
