// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import MainPanel from './components/main-panel'
import TreeList, { ToggleList } from './components/tree-list'
import {
  ViewType
} from './state/component_types'
import DataContext from './state/context'
import styled from 'styled-components'
import { getLocale } from '../../../common/locale'

const Box = styled.div`
  position: relative;
`

function Container () {
  const { siteBlockInfo, viewType, siteSettings } = React.useContext(DataContext)
  const shouldShowDetailView = viewType !== ViewType.Main && siteBlockInfo

  let treeListElement = null
  if (shouldShowDetailView) {
    if (viewType === ViewType.AdsList) {
      treeListElement = <TreeList
        blockedList={siteBlockInfo?.adsList}
        totalBlockedTitle={getLocale('braveShieldsTrackersAndAds')}
      />
    }

    if (viewType === ViewType.HttpsList) {
      treeListElement = <TreeList
        blockedList={ siteBlockInfo?.httpRedirectsList }
        totalBlockedTitle={getLocale('braveShieldsConnectionsUpgraded')}
      />
    }

    if (viewType === ViewType.ScriptsList) {
      treeListElement = <TreeList
          blockedList={ siteBlockInfo?.blockedJsList }
          allowedList={ siteBlockInfo?.allowedJsList }
          totalAllowedTitle={getLocale('braveShieldsAllowedScriptsLabel')}
          totalBlockedTitle={getLocale('braveShieldsBlockedScriptsLabel')}
        />
    }

    if (viewType === ViewType.FingerprintList) {
      treeListElement = <ToggleList
          webcompatSettings={ siteSettings?.webcompatSettings }
          totalBlockedTitle={getLocale('braveShieldsFingerprintingProtectionsAppliedLabel')}
          learnMoreText={getLocale('braveShieldsLearnMoreLinkText')}
          listDescription={getLocale('braveShieldsFingerprintingListDescription')}
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
