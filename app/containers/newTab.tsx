/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import * as newTabPageActions from '../actions/newTabPageActions'
import NewTab from '../components/newTab/newTab';

const mapStateToProps = (state: {newTabPage: any}) => ({
  newTabPage: state.newTabPage
})

const mapDispatchToProps = (dispatch: any) => ({
  actions: bindActionCreators(newTabPageActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(NewTab as any) // TODO remove any
