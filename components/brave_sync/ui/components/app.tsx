/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
// TODO: Add brave-ui components

// Utils
import * as syncActions from '../actions/sync_actions'

// Assets
require('../../../fonts/muli.css')
require('../../../fonts/poppins.css')

interface Props {
  syncData: Sync.State
  actions: any
}

export class SyncPage extends React.Component<Props, {}> {
  get actions () {
    return this.props.actions
  }

  doSomething = () => {
    this.actions.doSomethingSync('PIKACHU IS OVERRATED')
  }

  render () {
    return (
      <div id='syncPage'>
        <button onClick={this.doSomething}>action dispatched! check console</button>
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
