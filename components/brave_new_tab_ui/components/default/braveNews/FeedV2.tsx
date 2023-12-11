// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { getLocale } from '$web-common/locale'
import Button from '@brave/leo/react/button'
import { spacing } from '@brave/leo/tokens/css'
import * as React from 'react'
import styled, { css } from 'styled-components'
import Feed from '../../../../brave_news/browser/resources/Feed'
import FeedNavigation from '../../../../brave_news/browser/resources/FeedNavigation'
import Variables from '../../../../brave_news/browser/resources/Variables'
import { useBraveNews } from '../../../../brave_news/browser/resources/shared/Context'
import Flex from '../../../../common/Flex'
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
  bottom: ${spacing.xl};
  right: ${spacing.xl};

  opacity: calc((var(--ntp-scroll-percent) - 0.5) / 0.5);

  .${CLASSNAME_PAGE_STUCK} & {
    visibility: visible;
  }

  display: flex;
  gap: ${spacing.m};
`

// TODO: Remove from flow when hidden
const LoadNewContentButton = styled(Button) <{ show: boolean }>`
  position: sticky;
  top: ${spacing.xl};

  flex-grow: 0;

  opacity: 0;
  pointer-events: none;
  transition: opacity 0.2s ease-in-out;

  ${p => p.show && css`
    opacity: 1;
    pointer-events: unset;
  `}
`

export default function FeedV2() {
  const { feedV2, setCustomizePage, refreshFeedV2, feedV2UpdatesAvailable } = useBraveNews()

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

  // For some reason |createGlobalStyle| doesn't seem to work in Brave Core
  // To get the background blur effect looking nice, we need to set the body
  // background to black - unfortunately we can't do this in root HTML file
  // because we want to avoid the background flash effect.
  React.useEffect(() => {
    // Note: This is always black because this doesn't support light mode.
    document.body.style.backgroundColor = 'black';
  }, [])

  return <Root ref={ref as any} data-theme="dark">
    <SidebarContainer>
      <FeedNavigation />
    </SidebarContainer>
    <Flex align='center' direction='column' gap={spacing.l}>
      <LoadNewContentButton show={!!feedV2UpdatesAvailable} onClick={e => {
        refreshFeedV2()
        e.stopPropagation()
      }}>
        {getLocale('braveNewsNewContentAvailable')}
      </LoadNewContentButton>
      <Feed feed={feedV2} />
    </Flex>

    <ButtonsContainer>
      <Button kind='outline' onClick={() => setCustomizePage('news')}>
        {getLocale('braveNewsCustomizeFeed')}
      </Button>
      <Button isLoading={!feedV2} kind='outline' onClick={() => {
        refreshFeedV2()
      }}>{getLocale('braveNewsRefreshFeed')}</Button>
    </ButtonsContainer>
  </Root>
}
