/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import TipSite from './tipSite'
import TipMediaUser from './tipMediaUser'

// Utils
import * as rewardsActions from '../actions/tip_actions'

interface TipDialogArgs {
  url: string
  monthly: boolean
  publisherKey: string
  mediaMetaData?: RewardsTip.MediaMetaData
}

interface Props extends RewardsTip.ComponentProps {
  dialogArgs: TipDialogArgs
}

export class App extends React.Component<Props, {}> {
  get actions () {
    return this.props.actions
  }

  componentDidMount () {
    this.actions.onlyAnonWallet()
  }

  getTipBanner = (url: string, publisher: RewardsTip.Publisher, mediaMetaData: RewardsTip.MediaMetaData | undefined) => {
    let monthlyDate

    const {
      currentTipAmount,
      currentTipRecurring,
      reconcileStamp
    } = this.props.rewardsDonateData
    const monthly = this.props.dialogArgs.monthly

    if (currentTipRecurring && reconcileStamp) {
      monthlyDate = new Date(reconcileStamp * 1000).toLocaleDateString()
    }

    if (!mediaMetaData) {
      return (
        <TipSite
          url={url}
          monthly={monthly}
          publisher={publisher}
          monthlyDate={monthlyDate}
          amount={currentTipAmount}
        />
      )
    } else {
      return (
        <TipMediaUser
          url={url}
          monthly={monthly}
          publisher={publisher}
          mediaMetaData={mediaMetaData}
          monthlyDate={monthlyDate}
          amount={currentTipAmount}
        />
      )
    }
  }

  render () {
    const { publishers } = this.props.rewardsDonateData

    if (!publishers) {
      return null
    }

    const url = this.props.dialogArgs.url
    const mediaMetaData = this.props.dialogArgs.mediaMetaData
    const publisherKey = this.props.dialogArgs.publisherKey
    const publisher = publishers[publisherKey]

    if (!publisher) {
      return null
    }

    return (
      <div>
        {this.getTipBanner(url, publisher, mediaMetaData)}
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
