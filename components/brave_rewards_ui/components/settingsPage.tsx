/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {
  Column,
  Grid,
  MainToggle,
  SettingsPage as Page
} from 'brave-ui'
import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import PageWallet from './pageWallet'
import AdsBox from './adsBox'
import ContributeBox from './contributeBox'
import DonationBox from './donationsBox'

// Utils
import * as rewardsActions from '../actions/rewards_actions'

interface Props extends Rewards.ComponentProps {
}

class SettingsPage extends React.Component<Props, {}> {
  onToggle = () => {
    this.actions.onSettingSave('enabledMain', !this.props.rewardsData.enabledMain)
  }

  get actions () {
    return this.props.actions
  }

  componentDidMount () {
    if (this.props.rewardsData.firstLoad === null) {
      this.actions.onSettingSave('firstLoad', true)
    } else if (this.props.rewardsData.firstLoad) {
      this.actions.onSettingSave('firstLoad', false)
    }
  }

  render () {
    const { rewardsData } = this.props

    return (
      <Page>
        <Grid columns={3} theme={{ gridGap: '32px' }}>
          <Column size={2} theme={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            <MainToggle
              onToggle={this.onToggle}
              enabled={rewardsData.enabledMain}
            />
            <AdsBox />
            <ContributeBox />
            <DonationBox />
          </Column>
          <Column size={1}>
            <PageWallet />
          </Column>
        </Grid>
      </Page>
    )
  }
}

const mapStateToProps = (state: Rewards.ApplicationState) => ({
  rewardsData: state.rewardsData
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(SettingsPage)
