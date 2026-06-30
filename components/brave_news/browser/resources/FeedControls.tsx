// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import { spacing } from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import styled from 'styled-components'
import SettingsButton from './SettingsButton'
import { useBraveNews } from './shared/Context'

const SidebarMenu = React.lazy(() => import('./SidebarMenu'))

const Container = styled.div`
  max-width: min(540px, 100vw);

  display: flex;
  justify-content: flex-end;
  gap: ${spacing.m};

  margin-left: auto;
  margin-right: auto;
`

interface Props {
  // Invoked when the customize ("tune") button is pressed. Surfaces differ in
  // where customization lives: the feed opens its inline modal, while the side
  // panel opens the New Tab Page.
  onCustomize: () => void
  // Whether to show the feed-list menu. The feed only needs it on small
  // viewports (it has a dedicated sidebar otherwise), whereas narrow surfaces
  // like the side panel always show it.
  showMenu?: boolean
  className?: string
}

// The Brave News feed controls: an optional feed-list menu plus customize and
// refresh buttons. Shared by the feed page and the side panel so both expose
// the same actions; each surface supplies its own positioning container.
export default function FeedControls({
  onCustomize,
  showMenu,
  className,
}: Props) {
  const { feedV2, refreshFeedV2 } = useBraveNews()
  return (
    <Container className={className}>
      {showMenu && (
        <React.Suspense fallback={null}>
          <SidebarMenu />
        </React.Suspense>
      )}
      <SettingsButton
        onClick={onCustomize}
        title={getLocale(S.BRAVE_NEWS_CUSTOMIZE_FEED)}
      >
        <Icon name='tune' />
      </SettingsButton>
      <SettingsButton
        isLoading={!feedV2}
        title={getLocale(S.BRAVE_NEWS_REFRESH_FEED)}
        onClick={() => refreshFeedV2()}
      >
        <Icon name='refresh' />
      </SettingsButton>
    </Container>
  )
}
