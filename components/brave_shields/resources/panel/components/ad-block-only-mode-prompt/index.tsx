// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Alert from '@brave/leo/react/alert'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import { formatLocale, getLocale } from '$web-common/locale'
import {
  Container,
  Title,
  Description,
  Actions,
  ContainerNoTopPad,
  TitleCompact,
  DescriptionCompact
} from './style'

const handleLearnMoreClick = () => {
  chrome.tabs.create({ url: 'https://brave.com/privacy-features/', active: true })
}

const onDismissShieldsDisabledAdBlockOnlyModePromptClick = () => {
  getPanelBrowserAPI().dataHandler.setBraveShieldsAdBlockOnlyModePromptDismissed()
}

const onDismissRepeatedReloadsAdBlockOnlyModePromptClick = async () => {
  await getPanelBrowserAPI().dataHandler.setBraveShieldsAdBlockOnlyModePromptDismissed()
  getPanelBrowserAPI().panelHandler.closeUI()
}

const onEnableAdBlockOnlyModeClick = async (enableShields: boolean) => {
  if (enableShields) {
    await getPanelBrowserAPI().dataHandler.setBraveShieldsEnabled(/* isEnabled: */ true)
  }
  await getPanelBrowserAPI().dataHandler.setBraveShieldsAdBlockOnlyModeEnabled(/* isEnabled: */ true)
  getPanelBrowserAPI().panelHandler.closeUI()
}

export function AdBlockOnlyModePromptAfterRepeatedReloads() {
  return (
    <Container>
      <Title>
        {getLocale('braveShieldsAreYouExperiencingIssuesWithThisSiteTitle')}
      </Title>
      <Description>
        {formatLocale(`braveShieldsAreYouExperiencingIssuesWithThisSiteDesc`, {
          $1: content => <a href="#" onClick={handleLearnMoreClick}>{content}</a>
        })}
      </Description>
      <Actions>
        <Button
          kind="outline"
          size="medium"
          onClick={onDismissRepeatedReloadsAdBlockOnlyModePromptClick}
        >
          {getLocale('braveShieldsDismissAlert')}
        </Button>
        <Button
          size="medium"
          onClick={() => onEnableAdBlockOnlyModeClick(/* enableShields: */ false)}
        >
          {getLocale('braveShieldsEnableAdBlockOnlyMode')}
        </Button>
      </Actions>
    </Container>
  );
}

export function AdBlockOnlyModePromptAfterShieldsDisabled() {
  return (
    <ContainerNoTopPad>
      <Alert
        type='info'
        hideIcon
        hasActions={false}
      >
        <TitleCompact>
          {getLocale('braveShieldsAreYouExperiencingIssuesWithShieldsTitle')}
        </TitleCompact>
        <DescriptionCompact>
          {formatLocale(`braveShieldsAreYouExperiencingIssuesWithShieldsDesc`, {
            $1: content => <a href="#" onClick={handleLearnMoreClick}>{content}</a>
          })}
        </DescriptionCompact>
        <Actions>
          <Button
            size="medium"
            onClick={() => onEnableAdBlockOnlyModeClick(/* enableShields: */ true)}
          >
            {getLocale('braveShieldsEnableAdBlockOnlyMode')}
          </Button>
          <Button
            kind="plain"
            size="medium"
            onClick={() => onDismissShieldsDisabledAdBlockOnlyModePromptClick()}
          >
            {getLocale('braveShieldsDismissAlert')}
          </Button>
        </Actions>
      </Alert>
    </ContainerNoTopPad>
  )
}
