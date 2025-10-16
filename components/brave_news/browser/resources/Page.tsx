// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import { radius, spacing } from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import styled from 'styled-components'
import Feed from './Feed'
import NewsButton from './NewsButton'
import Variables from './Variables'
import { useBraveNews } from './shared/Context'
import SettingsButton from './SettingsButton'
import useMediaQuery from '$web-common/useMediaQuery'

const SidebarMenu = React.lazy(() => import('./SidebarMenu'))
const FeedNavigation = React.lazy(() => import('./FeedNavigation'))

const isSmallQuery = '(max-width: 1024px)'

const Root = styled(Variables)`
  --bn-top-bar-height: 78px;

  padding-top: ${spacing.xl};

  display: grid;
  grid-template-columns: 1fr max-content 1fr;
  gap: ${spacing['3Xl']};
`

const SidebarContainer = styled.div`
  position: sticky;
  top: ${spacing.xl};
`

const ButtonsContainer = styled.div`
  position: fixed;
  bottom: ${spacing['5Xl']};
  right: ${spacing['5Xl']};
  border-radius: ${radius.m};
  padding: ${spacing.m};

  background: var(--bn-glass-container);
  backdrop-filter: blur(64px);

  @media ${isSmallQuery} {
    height: var(--bn-top-bar-height);

    inset: 0;
    bottom: unset;
    padding: ${spacing['2Xl']} ${spacing.xl};
    border-radius: 0;
  }
`

const ButtonSpacer = styled.div`
  max-width: min(540px, 100vw);

  display: flex;
  justify-content: flex-end;
  gap: ${spacing.m};

  margin-left: auto;
  margin-right: auto;
`

const LoadNewContentButton = styled(NewsButton)`
  position: fixed;
  z-index: 1;
  top: ${spacing['3Xl']};

  flex-grow: 0;

  @media ${isSmallQuery} {
    top: calc(var(--bn-top-bar-height) + var(--leo-spacing-m));
  }
`

export default function FeedV2() {
  const isSmall = useMediaQuery(isSmallQuery)

  const { feedV2, setCustomizePage, refreshFeedV2, feedV2UpdatesAvailable, reportViewCount, reportSessionStart } = useBraveNews()
  const ref = React.useRef<HTMLDivElement>()

  // Note: Whenever the feed is updated, if we're viewing the feed, scroll to
  // the top.
  React.useLayoutEffect(() => {
    if (ref.current) {
      const rect = ref.current.getBoundingClientRect()
      if (rect.top < window.innerHeight / 2) {
        ref.current.scrollIntoView()
      }
    }
  }, [feedV2?.items])

  return <Root ref={ref as any} data-theme="dark">
    <SidebarContainer className='brave-news-sidebar'>
      {!isSmall && <React.Suspense fallback={null}><FeedNavigation /></React.Suspense>}
    </SidebarContainer>
    <Flex align='center' direction='column' gap={spacing.l}>
      {feedV2UpdatesAvailable &&
        <LoadNewContentButton
          className='brave-news-load-new-content-button'
          onClick={refreshFeedV2}
        >
          {getLocale(S.BRAVE_NEWS_NEW_CONTENT_AVAILABLE)}
        </LoadNewContentButton>}
      <Feed feed={feedV2} onViewCountChange={reportViewCount} onSessionStart={reportSessionStart} />
    </Flex>

    <ButtonsContainer className='brave-news-feed-controls'>
      <ButtonSpacer>
        {isSmall && <React.Suspense fallback={null}><SidebarMenu /></React.Suspense>}
        <SettingsButton onClick={() => setCustomizePage('news')} title={getLocale(S.BRAVE_NEWS_CUSTOMIZE_FEED)}>
          <Icon name="tune" />
        </SettingsButton>
        <SettingsButton isLoading={!feedV2} title={getLocale(S.BRAVE_NEWS_REFRESH_FEED)} onClick={() => {
          refreshFeedV2()
        }}><Icon name="refresh" /></SettingsButton>
      </ButtonSpacer>
    </ButtonsContainer>
  </Root>
}
