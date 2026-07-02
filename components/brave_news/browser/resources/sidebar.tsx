// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { setIconBasePath } from '@brave/leo/react/icon'
import { spacing } from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import { createRoot } from 'react-dom/client'
import StyledComponentsProvider from '$web-common/StyledComponentsProvider'
import styled from 'styled-components'
import Feed from './Feed'
import FeedControls from './FeedControls'
import Variables from './Variables'
import getBraveNewsController from './shared/api'
import { BraveNewsContextProvider, useBraveNews } from './shared/Context'
import './strings'

setIconBasePath('//resources/brave-icons')

const Root = styled(Variables)`
  /* Consumed by SidebarMenu to position its slide-out beneath the header. */
  --bn-top-bar-height: 56px;

  display: flex;
  flex-direction: column;
`

const Header = styled.div`
  position: sticky;
  top: 0;
  z-index: 1;

  height: var(--bn-top-bar-height);
  display: flex;
  align-items: center;
  gap: ${spacing.m};
  padding-inline: ${spacing.xl};

  background: var(--bn-glass-container);
  backdrop-filter: blur(64px);
`

const Content = styled.div`
  margin-inline: auto;
  width: 100%;
  max-width: 540px;
  padding: ${spacing.xl} ${spacing.m};

  .news-feed {
    width: 100%;
  }
`

// Fill the header so the controls' internal alignment (feed-list menu on the
// left, actions on the right) has the full width to spread across.
const Controls = styled(FeedControls)`
  flex: 1;
`

export function Sidebar() {
  const { feedV2, reportViewCount, reportSessionStart } = useBraveNews()
  return (
    <Root data-theme='dark'>
      <Header>
        <Controls
          onCustomize={() => getBraveNewsController().openSettings()}
          showMenu
        />
      </Header>
      <Content>
        <Feed
          feed={feedV2}
          onViewCountChange={reportViewCount}
          onSessionStart={reportSessionStart}
        />
      </Content>
    </Root>
  )
}

// Only mount when running as the WebUI page. Guarding the lookup keeps the
// module import-safe for Storybook, which renders <Sidebar /> directly.
const root = document.getElementById('root')
if (root) {
  createRoot(root).render(
    <StyledComponentsProvider>
      <BraveNewsContextProvider openArticlesInNewTab={false}>
        <Sidebar />
      </BraveNewsContextProvider>
    </StyledComponentsProvider>
  )
}
