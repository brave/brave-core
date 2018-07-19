/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { RegionalAdBlockEnabled } from './regionalAdBlockEnabled'
import { NumBlockedStat } from './numBlockedStat'

// Utils
import * as adblockActions from '../actions/adblock_actions'

interface Props {
  actions: any
  adblockData: AdBlock.State
}

export class AdblockPage extends React.Component<Props, {}> {
  get actions () {
    return this.props.actions
  }

  render () {
    const { adblockData } = this.props
    return (
      <div id='adblockPage'>
        <NumBlockedStat adsBlockedStat={adblockData.stats.adsBlockedStat || 0} />
        <RegionalAdBlockEnabled
          regionalAdBlockEnabled={adblockData.stats.regionalAdBlockEnabled}
          regionalAdBlockTitle={adblockData.stats.regionalAdBlockTitle || ''}
        />
      </div>)
  }
}

export const mapStateToProps = (state: AdBlock.ApplicationState) => ({
  adblockData: state.adblockData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(adblockActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(AdblockPage)
