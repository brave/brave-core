// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import { formatLocale, getLocale } from '$web-common/locale'
import {
  Container,
  Title,
  Description,
  Actions,
  StyledAlert,
  ActionsSlotWrapper,
} from './style'
import styles from '../alerts.module.scss'

const handleLearnMoreClick = (e: React.MouseEvent<HTMLAnchorElement>) => {
  e.preventDefault()
  chrome.tabs.create({ url: 'https://support.brave.app/hc/en-us/articles/38076796692109', active: true })
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
        <div>
          {formatLocale(`braveShieldsAreYouExperiencingIssuesWithThisSiteDesc1`, {
            $1: content => <a href='https://support.brave.app/hc/en-us/articles/38076796692109'
                              onClick={handleLearnMoreClick}>
              {content}
            </a>
          })}
        </div>
        <div>
          {getLocale('braveShieldsAreYouExperiencingIssuesWithThisSiteDesc2')}
        </div>
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
      <StyledAlert type='info' hideIcon>
        <div slot='title'>
          {getLocale('braveShieldsAreYouExperiencingIssuesWithThisSiteTitle')}
        </div>
        <div>
          {formatLocale(`braveShieldsAreYouExperiencingIssuesWithThisSiteDesc1`, {
            $1: content => <a href='https://support.brave.app/hc/en-us/articles/38076796692109'
                              onClick={handleLearnMoreClick}>
              {content}
            </a>
          })}
        </div>
        <div>
          {getLocale('braveShieldsAreYouExperiencingIssuesWithThisSiteDesc2')}
        </div>
        <ActionsSlotWrapper slot='actions' className={styles.actionsWrapper}>
          <div className={styles.buttonsWrapper}>
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
          </div>
        </ActionsSlotWrapper>
      </StyledAlert>
  )
}
