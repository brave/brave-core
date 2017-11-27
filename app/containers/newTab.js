/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import React, { Component } from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import * as newTabPageActions from '../actions/newTabPageActions'
import {getMessage} from '../background/api/localeAPI'

class NewTab extends Component {
  render () {
    // const { newTabPage, actions } = this.props
    const { actions } = this.props
    return <div data-test-id='new-tab-page'>
      <div>Hello, {getMessage('newTab')} world!</div>
      <div onClick={actions.settingsIconClicked}>Settings</div>
    </div>
  }
}

export default connect(
  state => ({
    newTabPage: state.newTabPage
  }),
  dispatch => ({
    actions: bindActionCreators(newTabPageActions, dispatch)
  })
)(NewTab)
