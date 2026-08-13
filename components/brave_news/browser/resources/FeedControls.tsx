// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import styled from 'styled-components'
import SettingsButton from './SettingsButton'
import { useBraveNews } from './shared/Context'

const SidebarMenu = React.lazy(() => import('./SidebarMenu'))

const Container = styled.div`
  max-width: min(540px, 100vw);
  width: 100%;

  display: flex;
  align-items: center;
  gap: ${spacing.m};
`

const RightSide = styled.div`
  display: flex;
  align-items: center;
  gap: ${spacing.m};
  margin-left: auto;
`

const Divider = styled.div`
  width: 1px;
  height: ${spacing.l};
  background: ${color.divider.subtle};
  flex: 0;
`

interface Props {
  // Invoked when the customize ("settings") button is pressed. Surfaces differ
  // in where customization lives: the feed opens its inline modal, while the
  // side panel opens the New Tab Page.
  onCustomize: () => void
  // Whether to show the feed-list menu. The feed only needs it on small
  // viewports (it has a dedicated sidebar otherwise), whereas narrow surfaces
  // like the side panel always show it.
  showMenu?: boolean
  // Optional heading rendered between the menu and the action buttons. Only the
  // side panel supplies this; the feed page has its own layout.
  title?: string
  // When provided, a close button is rendered (sidebar mode only). Closes the
  // hosting side panel.
  onClose?: () => void
  className?: string
}

const Title = styled.span`
  color: ${color.text.primary};
  font: ${font.heading.h4};
`

// The Brave News feed controls: an optional feed-list menu plus customize and
// refresh buttons. Shared by the feed page and the side panel so both expose
// the same actions; each surface supplies its own positioning container.
export default function FeedControls({
  onCustomize,
  showMenu,
  title,
  onClose,
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
      {title && <Title>{title}</Title>}
      <RightSide>
        <SettingsButton
          onClick={onCustomize}
          title={getLocale(S.BRAVE_NEWS_CUSTOMIZE_FEED)}
        >
          <Icon name='settings' />
        </SettingsButton>
        <SettingsButton
          isLoading={!feedV2}
          title={getLocale(S.BRAVE_NEWS_REFRESH_FEED)}
          onClick={() => refreshFeedV2()}
        >
          <Icon name='refresh' />
        </SettingsButton>
        {onClose && (
          <>
            <Divider />
            <SettingsButton
              onClick={onClose}
              title={getLocale(S.BRAVE_NEWS_CLOSE)}
            >
              <Icon name='close' />
            </SettingsButton>
          </>
        )}
      </RightSide>
    </Container>
  )
}
