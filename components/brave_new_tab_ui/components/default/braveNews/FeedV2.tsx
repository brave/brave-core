// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex'
import { getLocale } from '$web-common/locale'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { radius, spacing } from '@brave/leo/tokens/css'
import * as React from 'react'
import styled from 'styled-components'
import Feed from '../../../../brave_news/browser/resources/Feed'
import FeedNavigation from '../../../../brave_news/browser/resources/FeedNavigation'
import NewsButton from '../../../../brave_news/browser/resources/NewsButton'
import Variables from '../../../../brave_news/browser/resources/Variables'
import { useBraveNews } from '../../../../brave_news/browser/resources/shared/Context'
import { isPublisherEnabled } from '../../../../brave_news/browser/resources/shared/api'
import { CLASSNAME_PAGE_STUCK } from '../page'

const Root = styled(Variables)`
  padding-top: ${spacing.xl};

  display: grid;
  grid-template-columns: 1fr max-content 1fr;
  gap: ${spacing['3Xl']};
`

const SidebarContainer = styled.div`
  visibility: hidden;
  position: sticky;
  top: ${spacing.xl};

  opacity: calc(var(--ntp-extra-content-effect-multiplier));

  .${CLASSNAME_PAGE_STUCK} & {
    visibility: visible;
  }
`

const ButtonsContainer = styled.div`
  visibility: hidden;

  position: fixed;
  bottom: ${spacing['5Xl']};
  right: ${spacing['5Xl']};
  border-radius: ${radius.m};

  opacity: calc((var(--ntp-scroll-percent) - 0.5) / 0.5);

  .${CLASSNAME_PAGE_STUCK} & {
    visibility: visible;
  }

  display: flex;
  gap: ${spacing.m};
  padding: ${spacing.m};

  background: var(--bn-glass-container);
`

const SettingsButton = styled(Button)`
  --leo-button-color: var(--bn-glass-50);
  --leo-button-radius: ${radius.s};
  --leo-button-padding: ${spacing.m};
`

const LoadNewContentButton = styled(NewsButton)`
  position: fixed;
  z-index: 1;
  top: ${spacing['3Xl']};

  flex-grow: 0;
`

export default function FeedV2() {
  const { feedV2, setCustomizePage, refreshFeedV2, feedV2UpdatesAvailable, publishers, channels } = useBraveNews()

  // We don't want to decide whether we have subscriptions until the publishers
  // and channels have loaded.
  const loaded = React.useMemo(() => !!Object.values(publishers).length && !!Object.values(channels).length, [publishers, channels])

  // This is a bit of an interesting |useMemo| - we only want it to be updated
  // when the feed changes so as to not break the case where:
  // 1. The user has no feeds (we show the NoFeeds card)
  // 2. The user subscribes to a feed (we should still show the NoFeeds card,
  //    not the "Empty Feed")
  // To achieve this, |hasSubscriptions| is only updated when the feed changes,
  // or the opt-in status is changed.
  const hasSubscriptions = React.useMemo(() => !loaded
    || Object.values(publishers).some(isPublisherEnabled)
    || Object.values(channels).some(c => c.subscribedLocales.length), [feedV2, loaded])

  const ref = React.useRef<HTMLDivElement>()

  // Note: Whenever the feed is updated, if we're viewing the feed, scroll to
  // the top.
  React.useLayoutEffect(() => {
    const root = document.querySelector<HTMLElement>('#root')
    if (!root?.classList.contains(CLASSNAME_PAGE_STUCK)) {
      return
    }

    ref.current?.scrollIntoView()
  }, [feedV2?.items])

  return <Root ref={ref as any} data-theme="dark">
    <SidebarContainer>
      <FeedNavigation />
    </SidebarContainer>
    <Flex align='center' direction='column' gap={spacing.l}>
      {feedV2UpdatesAvailable && <LoadNewContentButton onClick={refreshFeedV2}>
        {getLocale('braveNewsNewContentAvailable')}
      </LoadNewContentButton>}
      <Feed feed={feedV2} hasSubscriptions={hasSubscriptions} />
    </Flex>

    <ButtonsContainer>
      <SettingsButton fab kind='outline' onClick={() => setCustomizePage('news')} title={getLocale('braveNewsCustomizeFeed')}>
        <Icon name="settings" />
      </SettingsButton>
      <SettingsButton fab isLoading={!feedV2} kind='outline' title={getLocale('braveNewsRefreshFeed')} onClick={() => {
        refreshFeedV2()
      }}><Icon name="refresh" /></SettingsButton>
    </ButtonsContainer>
  </Root>
}
