// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '$web-common/locale'
import {
  color,
  radius,
  spacing,
} from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import styled from 'styled-components'
import { useBraveNews } from '../shared/Context'
import DisabledPlaceholder from './DisabledPlaceholder'
import Discover from './Discover'
import NewsSettings from './NewsSettings'
import { PopularPage } from './Popular'
import SourcesList from './SourcesList'
import { SuggestionsPage } from './Suggestions'

const PanelBody = styled.div`
  display: flex;
  max-height: calc(100vh - ${spacing['4Xl']});
`

const Sidebar = styled.nav`
  flex: 0 0 228px;
  display: flex;
  flex-direction: column;
  gap: ${spacing['2Xl']};
  padding: ${spacing['2Xl']} 0;
  min-height: 0;
  overflow: auto;
  overscroll-behavior: contain;
  scrollbar-width: thin;
  white-space: nowrap;
`

const SidebarTitle = styled.h4`
  color: ${color.text.primary};
  padding: 0 ${spacing['2Xl']};
`

const Content = styled.section`
  flex: 1 1 auto;
  min-height: 0;
  padding: ${spacing.m} ${spacing.m} ${spacing.m} 0;
  overflow: auto;
  overscroll-behavior: contain;
  scrollbar-width: thin;
  display: flex;
  flex-direction: column;
`

const ContentCard = styled.div`
  flex-grow: 1;
  display: flex;
  flex-direction: column;
  gap: ${spacing['2Xl']};
  padding: ${spacing['4Xl']};
  border-radius: ${radius.xl};
  background: ${color.page.background};
`

export default function Configure() {
  const {
    customizePage,
    isOptInPrefEnabled,
    isShowOnNTPPrefEnabled,
  } = useBraveNews()

  // TODO(petemill): We'll probably need to have 2 toggles, or some other
  // way to know if brave news is "enabled" when Brave News is exposed
  // in places other than just the NTP. For now this is pretty tied to NTP.
  const isBraveNewsFullyEnabled = isOptInPrefEnabled && isShowOnNTPPrefEnabled

  let content: JSX.Element
  if (!isBraveNewsFullyEnabled) {
    content = <DisabledPlaceholder />
  } else if (customizePage === 'suggestions') {
    content = <SuggestionsPage />
  } else if (customizePage === 'popular') {
    content = <PopularPage />
  } else {
    content = <Discover />
  }

  return (
    <div id='brave-news-configure'>
      <PanelBody>
        <Sidebar>
          <SidebarTitle>{getLocale(S.BRAVE_NEWS_SETTINGS_TITLE)}</SidebarTitle>
          {isBraveNewsFullyEnabled && <SourcesList />}
        </Sidebar>
        <Content>
          <ContentCard>
            {customizePage !== 'suggestions' && customizePage !== 'popular' &&
              <NewsSettings />}
            {content}
          </ContentCard>
        </Content>
      </PanelBody>
    </div>
  )
}
