// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import NewPrivateTabPage from './privateTab'
import NewTabPage from './newTab'

// Utils
import * as newTabActions from '../actions/new_tab_actions'
import * as PreferencesAPI from '../api/preferences'

interface Props {
  actions: any
  newTabData: NewTab.State
}

class DefaultPage extends React.Component<Props, {}> {
  render () {
    const { newTabData, actions } = this.props

    // don't render if user prefers an empty page
    if (this.props.newTabData.showEmptyPage && !this.props.newTabData.isIncognito) {
      return <div />
    }

    return this.props.newTabData.isIncognito
      ? <NewPrivateTabPage newTabData={newTabData} actions={actions} />
      : (
        <NewTabPage
          newTabData={newTabData}
          actions={actions}
          saveShowBackgroundImage={PreferencesAPI.saveShowBackgroundImage}
          saveShowClock={PreferencesAPI.saveShowClock}
          saveShowStats={PreferencesAPI.saveShowStats}
          saveShowTopSites={PreferencesAPI.saveShowTopSites}
          saveShowRewards={PreferencesAPI.saveShowRewards}
        />
      )
  }
}

const mapStateToProps = (state: NewTab.ApplicationState) => ({
  newTabData: state.newTabData
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(newTabActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(DefaultPage)
