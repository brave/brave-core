/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { GeneralInfo } from './generalInfo'

// Utils
import * as torInternalsActions from '../actions/tor_internals_actions'

interface Props {
  actions: any
  torInternalsData: TorInternals.State
}

export class TorInternalsPage extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  get actions () {
    return this.props.actions
  }

  render () {
    return (
      <div id='Page'>
        <GeneralInfo state={this.props.torInternalsData} />
      </div>
    )
  }
}

export const mapStateToProps = (state: TorInternals.ApplicationState) => ({
  torInternalsData: state.torInternalsData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(torInternalsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(TorInternalsPage)

