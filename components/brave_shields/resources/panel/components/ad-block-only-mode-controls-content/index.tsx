// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import Icon, { setIconBasePath } from '@brave/leo/react/icon'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import { getLocale } from '$web-common/locale'
import { GlobalSettings } from "../advanced-controls-content"
import {
  Container,
  HeaderRow,
  HeaderTextColumn,
  HeaderTitle,
  HeaderDescription,
  ActionsSlotWrapper,
} from './style'

setIconBasePath('chrome://resources/brave-icons')

const onSettingsClick = () => {
  chrome.tabs.create({ url: 'chrome://settings/shields', active: true })
}

function IsTheSiteWorkingCorrectlyNowAdBlockOnlyModeNotice() {
  const onLooksGoodClick = () => {
    getPanelBrowserAPI().panelHandler.closeUI()
  }

  const onReportClick = () => {
    getPanelBrowserAPI().dataHandler.openWebCompatWindow()
  }

  return (
    <Container>
      <HeaderRow>
        <HeaderTextColumn>
          <HeaderTitle>
            {getLocale('braveShieldsAdBlockOnlyModeEnabledTitle')}
          </HeaderTitle>
          <HeaderDescription>
            {getLocale('braveShieldsAdBlockOnlyModeEnabledDesc')}
          </HeaderDescription>
        </HeaderTextColumn>
        <Button
          kind='plain-faint'
          size='small'
          onClick={onSettingsClick}
        >
          <Icon name='launch' />
        </Button>
      </HeaderRow>
      <Alert type='info' hideIcon>
        <div slot='title'>
          {getLocale('braveShieldsIsThisSiteWorkingCorrectlyNowTitle')}
        </div>
        <div>
          {getLocale('braveShieldsIsThisSiteWorkingCorrectlyNowDesc')}
        </div>
        <ActionsSlotWrapper slot='actions'>
          <Button size="medium" onClick={onLooksGoodClick}>
            {getLocale('braveShieldsIsThisSiteWorkingCorrectlyNowLooksGood')}
          </Button>
          <Button kind="plain" size="medium" onClick={onReportClick}>
            {getLocale('braveShieldsIsThisSiteWorkingCorrectlyNowReportSite')}
          </Button>
        </ActionsSlotWrapper>
      </Alert>
    </Container>
  )
}

function AdBlockOnlyModeControlsContent(
    { showGlobalSettings }: { showGlobalSettings: boolean }) {
  return (
    <section id='global-controls-content'>
      <IsTheSiteWorkingCorrectlyNowAdBlockOnlyModeNotice />
      {showGlobalSettings && <GlobalSettings />}
    </section>
  )
}

export default AdBlockOnlyModeControlsContent
