/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Component groups
import DisabledContent from './disabledContent'
import EnabledContent from './enabledContent'

// Utils
import * as syncActions from '../actions/sync_actions'

// Assets
require('../../../../ui/webui/resources/fonts/muli.css')
require('../../../../ui/webui/resources/fonts/poppins.css')
require('emptykit.css')

interface Props {
  syncData: Sync.State
  actions: any
}

export class SyncPage extends React.PureComponent<Props, {}> {
  componentDidMount () {
    // Inform the back-end that Sync can be loaded
    this.props.actions.onPageLoaded()
  }

  render () {
    const { syncData, actions } = this.props

    if (!syncData) {
      return null
    }

    return (
      <div id='syncPage'>
        {
          syncData.isSyncConfigured && syncData.devices.length > 1
            ? <EnabledContent syncData={syncData} actions={actions} />
            : <DisabledContent syncData={syncData} actions={actions} />
        }
      </div>
    )
  }
}

export const mapStateToProps = (state: Sync.ApplicationState) => ({
  syncData: state.syncData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(syncActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(SyncPage)
