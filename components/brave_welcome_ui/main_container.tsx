// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import DataContext from './state/context'
import { useShouldPlayAnimations } from './state/hooks'
import { ViewType } from './state/component_types'

import HelpImprove from './components/help-improve'
import ImportInProgress from './components/import-in-progress'
import SelectBrowser from './components/select-browser'
import SelectProfile from './components/select-profile'
import SetupComplete from './components/setup-complete'
import Background from './components/background'
import Welcome from './components/welcome'

function MainContainer () {
  const { viewType } = React.useContext(DataContext)
  const shouldPlayAnimations = useShouldPlayAnimations()

  let mainEl = <Welcome />

  if (viewType === ViewType.SelectBrowser) {
    mainEl = <SelectBrowser />
  }

  if (viewType === ViewType.SelectProfile) {
    mainEl = <SelectProfile />
  }

  if (viewType === ViewType.ImportInProgress) {
    mainEl = <ImportInProgress />
  }

  if (viewType === ViewType.ImportSucceeded) {
    mainEl = <SetupComplete />
  }

  if (viewType === ViewType.ImportFailed) {
    mainEl = <p>Failed...</p>
  }

  if (viewType === ViewType.HelpImprove) {
    mainEl = <HelpImprove />
  }

  return (
    <Background static={!shouldPlayAnimations}>
      {mainEl}
    </Background>
  )
}

export default MainContainer
