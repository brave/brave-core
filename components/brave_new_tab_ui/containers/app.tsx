// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import NewPrivateTabPage from './privateTab'
import NewTabPage from './newTab'

// Utils
import * as PreferencesAPI from '../api/preferences'
import { getActionsForDispatch } from '../api/getActions'

// Types
import { NewTabActions } from '../constants/new_tab_types'
import { ApplicationState } from '../reducers'
import { BraveTodayState } from '../reducers/today'

interface Props {
  actions: NewTabActions
  newTabData: NewTab.State
  gridSitesData: NewTab.GridSitesState
  braveTodayData: BraveTodayState
}

class DefaultPage extends React.Component<Props, {}> {
  render () {
    const { newTabData, braveTodayData, gridSitesData, actions } = this.props

    // don't render if user prefers an empty page
    if (this.props.newTabData.showEmptyPage && !this.props.newTabData.isIncognito) {
      return <div />
    }

    return this.props.newTabData.isIncognito
      ? <NewPrivateTabPage newTabData={newTabData} actions={actions} />
      : (
        <NewTabPage
          newTabData={newTabData}
          todayData={braveTodayData}
          gridSitesData={gridSitesData}
          actions={actions}
          saveShowBackgroundImage={PreferencesAPI.saveShowBackgroundImage}
          saveShowStats={PreferencesAPI.saveShowStats}
          saveShowToday={PreferencesAPI.saveShowToday}
          saveShowRewards={PreferencesAPI.saveShowRewards}
          saveShowTogether={PreferencesAPI.saveShowTogether}
          saveShowBinance={PreferencesAPI.saveShowBinance}
          saveShowAddCard={PreferencesAPI.saveShowAddCard}
          saveShowGemini={PreferencesAPI.saveShowGemini}
          saveShowBitcoinDotCom={PreferencesAPI.saveShowBitcoinDotCom}
          saveShowCryptoDotCom={PreferencesAPI.saveShowCryptoDotCom}
          saveBrandedWallpaperOptIn={PreferencesAPI.saveBrandedWallpaperOptIn}
        />
      )
  }
}

const mapStateToProps = (state: ApplicationState): Partial<Props> => ({
  newTabData: state.newTabData,
  gridSitesData: state.gridSitesData,
  braveTodayData: state.today
})

const mapDispatchToProps = (dispatch: Dispatch): Partial<Props> => {
  return {
    actions: getActionsForDispatch(dispatch)
  }
}

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(DefaultPage)
