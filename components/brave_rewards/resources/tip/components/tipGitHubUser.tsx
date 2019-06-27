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
import { getLocale } from '../../../../common/locale'

interface Props extends RewardsTip.ComponentProps {
  url: string
  publisher: RewardsTip.Publisher
  githubMetaData: RewardsTip.GitHubMetaData
}

class TipGitHubUser extends React.Component<Props, {}> {
  get actions () {
    return this.props.actions
  }

  onTweet = () => {
    let name = this.props.publisher.name

    this.actions.onTweet(name, '')
    this.actions.onCloseDialog()
  }

  render () {
    const { finished, error } = this.props.rewardsDonateData

    const publisher = this.props.publisher
    const githubMetaData = this.props.githubMetaData
    const key = githubMetaData ? 'githubTipTitle' : 'githubTipTitleEmpty'
    publisher.title = getLocale(key, { user: '@' + githubMetaData.userName })

    return (
      <>
        {
          !finished && !error
            ? <Banner publisher={publisher} githubMetaData={githubMetaData} />
            : null
        }
        {
          finished
            ? <TransientTipOverlay
              publisher={publisher}
              onTweet={this.onTweet}
              timeout={3000}
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
)(TipGitHubUser)
