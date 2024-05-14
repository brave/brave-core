// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { addWebUiListener } from 'chrome://resources/js/cr.js'

import DataContext, { Store } from '../state/context'
import { ViewType, Scenes } from '../state/component_types'
import {
  useInitializeImportData,
  useProfileCount,
  useViewTypeTransition
} from '../state/hooks'

interface DataContextProviderProps {
  children: React.ReactNode
}

function DataContextProvider (props: DataContextProviderProps) {
  const [viewType, setViewType] = React.useState<ViewType | undefined>(
    undefined
  )
  const [currentSelectedBrowser, setCurrentSelectedBrowser] = React.useState<
    string | undefined
  >(undefined)
  const { browserProfiles } = useInitializeImportData()
  const { profileCountRef, incrementCount, decrementCount } = useProfileCount()
  const [scenes, setScenes] = React.useState<Scenes | undefined>(undefined)

  const currentSelectedBrowserProfiles = React.useMemo(() => {
    return browserProfiles?.filter(
      (profile) => profile.browserType === currentSelectedBrowser
    )
  }, [browserProfiles, currentSelectedBrowser])

  const store: Store = {
    viewType,
    setViewType,
    incrementCount,
    browserProfiles,
    currentSelectedBrowser,
    setCurrentSelectedBrowser,
    currentSelectedBrowserProfiles,
    scenes,
    setScenes
  }

  const importInProgress = useViewTypeTransition(
    ViewType.ImportSelectProfile
  ).forward
  const importSucceeded = useViewTypeTransition(
    ViewType.ImportInProgress
  ).forward

  React.useEffect(() => {
    addWebUiListener('brave-import-data-status-changed', (status: any) => {
      // TODO(tali): Handle item based events

      if (status.event === 'ImportStarted' && profileCountRef.current > 0) {
        setViewType(importInProgress)
      }

      if (status.event === 'ImportEnded') {
        decrementCount()
        if (profileCountRef.current === 0) {
          setViewType(importSucceeded)
        }
      }
    })
  }, [importSucceeded, importInProgress])

  return (
    <DataContext.Provider value={store}>{props.children}</DataContext.Provider>
  )
}

export default DataContextProvider
