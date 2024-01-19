// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import { radius, spacing } from '@brave/leo/tokens/css'
import * as React from 'react'
import styled from 'styled-components'
import Feed from '../../../../brave_news/browser/resources/Feed'
import NewsButton from '../../../../brave_news/browser/resources/NewsButton'
import Variables from '../../../../brave_news/browser/resources/Variables'
import { useBraveNews } from '../../../../brave_news/browser/resources/shared/Context'
import { CLASSNAME_PAGE_STUCK } from '../page'
import SettingsButton from '../../../../brave_news/browser/resources/SettingsButton'
import useMediaQuery from '$web-common/useMediaQuery'

const SidebarMenu = React.lazy(() => import('./SidebarMenu'))
const FeedNavigation = React.lazy(() => import('../../../../brave_news/browser/resources/FeedNavigation'))

const isSmallQuery = '(max-width: 1024px)'

const Root = styled(Variables)`
  --bn-top-bar-height: 78px;

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
    const root = document.querySelector<HTMLElement>('#root')
    if (!root?.classList.contains(CLASSNAME_PAGE_STUCK)) {
      return
    }

    ref.current?.scrollIntoView()
  }, [feedV2?.items])

  return <Root ref={ref as any} data-theme="dark">
    <SidebarContainer>
      {!isSmall && <React.Suspense fallback={null}><FeedNavigation /></React.Suspense>}
    </SidebarContainer>
    <Flex align='center' direction='column' gap={spacing.l}>
      {feedV2UpdatesAvailable && <LoadNewContentButton onClick={refreshFeedV2}>
        {getLocale('braveNewsNewContentAvailable')}
      </LoadNewContentButton>}
      <Feed feed={feedV2} onViewCountChange={reportViewCount} onSessionStart={reportSessionStart} />
    </Flex>

    <ButtonsContainer>
      <ButtonSpacer>
        {isSmall && <React.Suspense fallback={null}><SidebarMenu /></React.Suspense>}
        <SettingsButton onClick={() => setCustomizePage('news')} title={getLocale('braveNewsCustomizeFeed')}>
          <Icon name="tune" />
        </SettingsButton>
        <SettingsButton isLoading={!feedV2} title={getLocale('braveNewsRefreshFeed')} onClick={() => {
          refreshFeedV2()
        }}><Icon name="refresh" /></SettingsButton>
      </ButtonSpacer>
    </ButtonsContainer>
  </Root>
}
