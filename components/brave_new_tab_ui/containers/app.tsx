// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Dispatch } from 'redux'
import { connect } from 'react-redux'

import '$web-components/app.global.scss'

// Components
import NewTabPage from './newTab'

// Utils
import * as PreferencesAPI from '../api/preferences'
import getBraveNewsController from '../api/brave_news'
import getNTPBrowserAPI from '../api/background'
import { getActionsForDispatch } from '../api/getActions'

// Types
import { NewTabActions } from '../constants/new_tab_types'
import { ApplicationState } from '../reducers'
import { BraveNewsState } from '../reducers/today'

interface Props {
  actions: NewTabActions
  newTabData: NewTab.State
  gridSitesData: NewTab.GridSitesState
  braveNewsData: BraveNewsState
}

const getBraveNewsDisplayAd = function GetBraveNewsDisplayAd () {
  return getBraveNewsController().getDisplayAd()
}

function DefaultPage (props: Props) {
  const { newTabData, braveNewsData, gridSitesData, actions } = props

  // don't render if user prefers an empty page
  if (props.newTabData.showEmptyPage && !props.newTabData.isIncognito) {
    return <div />
  }

  return (
    <NewTabPage
      newTabData={newTabData}
      todayData={braveNewsData}
      gridSitesData={gridSitesData}
      actions={actions}
      saveShowBackgroundImage={PreferencesAPI.saveShowBackgroundImage}
      saveShowRewards={PreferencesAPI.saveShowRewards}
      saveShowBraveTalk={PreferencesAPI.saveShowBraveTalk}
      saveBrandedWallpaperOptIn={PreferencesAPI.saveBrandedWallpaperOptIn}
      saveSetAllStackWidgets={PreferencesAPI.saveSetAllStackWidgets}
      getBraveNewsDisplayAd={getBraveNewsDisplayAd}
      chooseNewCustomBackgroundImage={() => getNTPBrowserAPI().pageHandler.chooseLocalCustomBackground() }
      setCustomImageBackground={background => getNTPBrowserAPI().pageHandler.useCustomImageBackground(background) }
      removeCustomImageBackground={background => getNTPBrowserAPI().pageHandler.removeCustomImageBackground(background) }
      setBraveBackground={selectedBackground => getNTPBrowserAPI().pageHandler.useBraveBackground(selectedBackground)}
      setColorBackground={(color, useRandomColor) => getNTPBrowserAPI().pageHandler.useColorBackground(color, useRandomColor) }
    />
  )
}

const mapStateToProps = (state: ApplicationState): Partial<Props> => ({
  newTabData: state.newTabData,
  gridSitesData: state.gridSitesData,
  braveNewsData: state.today
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
