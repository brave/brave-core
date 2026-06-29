// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { setIconBasePath } from '@brave/leo/react/icon'
import { spacing } from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import { createRoot } from 'react-dom/client'
import styled from 'styled-components'
import Feed from './Feed'
import Variables from './Variables'
import { BraveNewsContextProvider, useBraveNews } from './shared/Context'
import './strings'

setIconBasePath('//resources/brave-icons')

const Root = styled(Variables)`
  margin-inline: auto;
  max-width: 540px;
  padding: ${spacing.xl} ${spacing.m};

  .news-feed {
    width: 100%;
  }
`

export function Sidebar() {
  const { feedV2, reportViewCount, reportSessionStart } = useBraveNews()
  return (
    <Root data-theme='dark'>
      <Feed
        feed={feedV2}
        onViewCountChange={reportViewCount}
        onSessionStart={reportSessionStart}
      />
    </Root>
  )
}

// Only mount when running as the WebUI page. Guarding the lookup keeps the
// module import-safe for Storybook, which renders <Sidebar /> directly.
const root = document.getElementById('root')
if (root) {
  createRoot(root).render(
    <BraveNewsContextProvider openArticlesInNewTab={false}>
      <Sidebar />
    </BraveNewsContextProvider>
  )
}
