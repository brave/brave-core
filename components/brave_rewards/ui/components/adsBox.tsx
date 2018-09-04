/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Box, DisabledContent } from 'brave-ui/features/rewards'

// Utils
import { getLocale } from '../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'

// Assets
const adsDisabledIcon = require('../../../img/rewards/ads_disabled.svg')

class AdsBox extends React.Component {
  adsDisabled () {
    return (
      <DisabledContent
        image={adsDisabledIcon}
        type={'ads'}
      >
        <h3>{getLocale('adsDisabledText')}</h3>
      </DisabledContent>
    )
  }

  render () {
    return (
      <Box
        title={getLocale('adsTitle')}
        type={'ads'}
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
