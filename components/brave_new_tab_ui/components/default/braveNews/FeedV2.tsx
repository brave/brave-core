// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { spacing } from '@brave/leo/tokens/css'
import * as React from 'react'
import styled from 'styled-components'
import Feed from '../../../../brave_news/browser/resources/Feed'
import Sidebar from '../../../../brave_news/browser/resources/Sidebar'
import { useBraveNews } from '../../../../brave_news/browser/resources/shared/Context'
import { CLASSNAME_PAGE_STUCK } from '../page'

const Variables = styled.div`
  --bn-text-base: 0, 0, 0;

  @media (prefers-color-scheme: dark) {
    --bn-text-base: 255, 255, 255;
  }

  padding-top: ${spacing.xl};
`

const SidebarContainer = styled.div`
  visibility: hidden;
  position: fixed;
  top: ${spacing.xl};
  right: ${spacing.xl};

  opacity: calc((var(--ntp-extra-content-effect-multiplier) - 0.5) * 2);

  .${CLASSNAME_PAGE_STUCK} & {
    visibility: visible;
  }
`

export default function FeedV2() {
  const { feedV2 } = useBraveNews()

  const ref = React.useRef<HTMLDivElement>()

  // Note: Whenever the feed is updated, if we're viewing the feed, scroll to
  // the top.
  React.useEffect(() => {
    const root = document.querySelector<HTMLElement>('#root')
    if (!root?.classList.contains(CLASSNAME_PAGE_STUCK)) {
      return
    }

    ref.current?.scrollIntoView()
  }, [feedV2?.items])

  return <Variables ref={ref as any}>
    <Feed feed={feedV2} />
    <SidebarContainer>
      <Sidebar />
    </SidebarContainer>
  </Variables>
}
