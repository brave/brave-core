/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import WebcompatReportModal from '../components/WebcompatReportModal'

// Utils
import * as webcompatReporterActions from '../actions/webcompatreporter_actions'

interface Props {
  actions: any
  reporterState: WebcompatReporter.State
}

class WebcompatReportContainer extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  get actions () {
    return this.props.actions
  }

  render () {
    const { actions, reporterState } = this.props

    return (
      <WebcompatReportModal
        siteUrl={reporterState.dialogArgs.url}
        contactInfo={reporterState.dialogArgs.contactInfo}
        isErrorPage={reporterState.dialogArgs.isErrorPage}
        submitted={reporterState.submitted}
        onSubmitReport={actions.onSubmitReport}
        onClose={actions.onClose}
      />
    )
  }
}

const mapStateToProps = (state: WebcompatReporter.ApplicationState) => ({
  reporterState: state.reporterState
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(webcompatReporterActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(WebcompatReportContainer)
