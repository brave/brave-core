// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import Flex from '$web-common/Flex'
import Discover from './Discover'
import { BackArrow } from '../../../../../brave_news/browser/resources/shared/Icons'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import Toggle from '@brave/leo/react/toggle'
import SourcesList from './SourcesList'
import DisabledPlaceholder from './DisabledPlaceholder'
import { useBraveNews } from '../../../../../brave_news/browser/resources/shared/Context'
import { getLocale } from '$web-common/locale'
import { formatMessage } from '../../../../../brave_rewards/resources/shared/lib/locale_context'
import { SuggestionsPage } from './Suggestions'
import { PopularPage } from './Popular'
import { spacing } from '@brave/leo/tokens/css'

const Grid = styled.div`
  width: 100%;
  min-width: 730px;
  height: 100%;

  overflow: auto;
  overscroll-behavior: none;

  display: grid;
  grid-template-columns: 307px auto;
  grid-template-rows: 64px 2px auto;

  grid-template-areas:
    "back-button header"
    "separator separator"
    "sidebar content";
`

const Header = styled(Flex)`
  grid-area: header;
  padding: 24px;
`

const HeaderText = styled.span`
  font-size: 16px;
  font-weight: 500;
`

const BackButtonContainer = styled.div`
  grid-area: back-button;
  align-items: center;
  display: flex;
  padding: 12px;
  padding-left: 34px;
`

const CloseButton = styled(Button)`
  flex-grow: 0;
`

const Hr = styled.hr`
  grid-area: separator;
  width: 100%;
  align-self: center;
  background: var(--divider1);
  height: 2px;
  border-width: 0;
`

const Sidebar = styled.div`
  position: relative;
  overflow: auto;
  grid-area: sidebar;
  padding: 28px 22px 28px 32px;
  background: var(--background2);
`

// Overlay on top of the sidebar, shown when it is disabled.
const SidebarOverlay = styled.div`
  position: absolute;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  background: var(--background1);
  opacity: 0.7;
`

const Content = styled.div`
  grid-area: content;
  overflow: auto;
  padding: 20px 64px;
`

export default function Configure() {
  const {
    setCustomizePage,
    customizePage,
    toggleBraveNewsOnNTP,
    isOptInPrefEnabled,
    isShowOnNTPPrefEnabled
  } = useBraveNews()

  // TODO(petemill): We'll probably need to have 2 toggles, or some other
  // way to know if brave news is "enabled" when Brave News is exposed
  // in places other than just the NTP. For now this is pretty tied to NTP.
  const isBraveNewsFullyEnabled = isOptInPrefEnabled && isShowOnNTPPrefEnabled

  let content: JSX.Element
  if (!isBraveNewsFullyEnabled) {
    content = <DisabledPlaceholder enableBraveNews={() => toggleBraveNewsOnNTP(true)} />
  } else if (customizePage === 'suggestions') {
    content = <SuggestionsPage />
  } else if (customizePage === 'popular') {
    content = <PopularPage />
  } else {
    content = <Discover />
  }

  return (
    <Grid id='brave-news-configure'>
      <BackButtonContainer>
        <Button onClick={() => setCustomizePage(null)} kind='plain-faint'>
          <Flex direction='row' align='center' gap={spacing[8]}>
            {BackArrow}
            <span>
              {formatMessage(getLocale('braveNewsBackToDashboard'), {
                tags: {
                  $1: content => <strong key="$1">{content}</strong>
                }
              })}
            </span>
          </Flex>
        </Button>
      </BackButtonContainer>
      <Header direction="row-reverse" gap={12} align="center" justify="space-between">
        <CloseButton onClick={() => setCustomizePage(null)} kind='plain-faint'>
          <Icon name='close' />
        </CloseButton>
        {isBraveNewsFullyEnabled && <Flex direction="row" align="center" gap={8}>
          <HeaderText>{getLocale('braveNewsTitle')}</HeaderText>
          <Toggle checked={isShowOnNTPPrefEnabled} onChange={e => toggleBraveNewsOnNTP(e.detail.checked)} />
        </Flex>}
      </Header>
      <Hr />
      <Sidebar>
        <SourcesList />
        {!isBraveNewsFullyEnabled && <SidebarOverlay />}
      </Sidebar>
      <Content>
        {content}
      </Content>
    </Grid>
  )
}
