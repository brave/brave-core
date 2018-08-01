/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Box, DisabledContent } from 'brave-ui/features/rewards'
import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Utils
import { getLocale } from '../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'

// Assets
const adsDisabledIcon = require('../../img/rewards/ads_disabled.svg')

class AdsBox extends React.Component {
  adsDisabled () {
    return (
      <DisabledContent
        image={adsDisabledIcon}
        theme={{ color: '#ceb4e1', boldColor: '#b490cf' }}
      >
        <h3>{getLocale('adsDisabledText')}</h3>
      </DisabledContent>
    )
  }

  render () {
    return (
      <Box
        title={getLocale('adsTitle')}
        theme={{ titleColor: '#C12D7C' }}
        description={getLocale('adsDesc')}
        toggle={false}
        disabledContent={this.adsDisabled()}
      />
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
)(AdsBox)
