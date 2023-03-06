// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import MainPanel from './components/main-panel'
import TreeList from './components/tree-list'
import {
  MakeResourceInfoList,
  ResourceInfo,
  ResourceState,
  ResourceType,
  ViewType
} from './state/component_types'
import DataContext from './state/context'
import styled from 'styled-components'
import { getLocale } from '../../../common/locale'
import { SiteBlockInfo } from './api/panel_browser_api'

const Box = styled.div`
  position: relative;
`

function GetResourceListForView (view: ViewType,
                                 state: ResourceState,
                                 siteBlockInfo: SiteBlockInfo): ResourceInfo[] {
  if (view === ViewType.AdsList && state === ResourceState.Blocked) {
    return MakeResourceInfoList(siteBlockInfo.adsList, ResourceType.Ad, state)
  }
  if (view === ViewType.HttpsList && state === ResourceState.Blocked) {
    return MakeResourceInfoList(siteBlockInfo.httpRedirectsList,
                                ResourceType.Http,
                                state)
  }
  if (view === ViewType.ScriptsList && state === ResourceState.Blocked) {
    return MakeResourceInfoList(siteBlockInfo.blockedJsList,
                                ResourceType.Http,
                                state)
  }
  if (view === ViewType.ScriptsList && state === ResourceState.AllowedOnce) {
    return MakeResourceInfoList(siteBlockInfo.allowedJsList,
                                ResourceType.Http,
                                state)
  }

  return []
}

function Container () {
  const { siteBlockInfo, viewType } = React.useContext(DataContext)
  const shouldShowDetailView = viewType !== ViewType.Main && siteBlockInfo

  const blockedList = siteBlockInfo ?
    GetResourceListForView(viewType, ResourceState.Blocked, siteBlockInfo) : []
  const allowedList = siteBlockInfo ?
    GetResourceListForView(viewType,
                           ResourceState.AllowedOnce,
                           siteBlockInfo) : []
  let treeListElement = null

  if (shouldShowDetailView) {
    if (viewType === ViewType.AdsList) {
      treeListElement = <TreeList
        resourcesList={{ allowedList, blockedList }}
        type={ResourceType.Ad}
        totalAllowedTitle=''
        totalBlockedTitle={getLocale('braveShieldsTrackersAndAds')}
      />
    }

    if (viewType === ViewType.HttpsList) {
      treeListElement = <TreeList
        type={ResourceType.Http}
        resourcesList={{ allowedList, blockedList }}
        totalAllowedTitle=''
        totalBlockedTitle={getLocale('braveShieldsConnectionsUpgraded')}
      />
    }

    if (viewType === ViewType.ScriptsList) {
      treeListElement = <TreeList
          resourcesList={{ allowedList, blockedList }}
          type={ResourceType.Script}
          totalAllowedTitle={getLocale('braveShieldsAllowedScriptsLabel')}
          totalBlockedTitle={getLocale('braveShieldsBlockedScriptsLabel')}
        />
    }
  }

  return (
    <Box>
      {treeListElement}
      <MainPanel />
    </Box>
  )
}

export default Container
