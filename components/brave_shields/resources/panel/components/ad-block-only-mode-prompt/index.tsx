// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Alert from '@brave/leo/react/alert'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import { formatLocale, getLocale } from '$web-common/locale'

const handleLearnMoreClick = () => {
  chrome.tabs.create({ url: 'https://brave.com/privacy-features/', active: true })
}

const onDismissShieldsDisabledAdBlockOnlyModePromptClick = (onDismiss?: () => void) => {
  getPanelBrowserAPI().dataHandler.setBraveShieldsAdBlockOnlyModePromptDismissed()
  if (onDismiss) {
    onDismiss()
  }
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
    <div style={{ padding: 'var(--leo-spacing-xl)' }}>
      <div style={{ font: 'var(--leo-font-heading-h4)' }}>
        {getLocale('braveShieldsAreYouExperiencingIssuesWithThisSiteTitle')}
      </div>
      <div style={{ padding: 'var(--leo-spacing-xl) 0', font: 'var(--leo-font-default-regular)' }}>
        {formatLocale(`braveShieldsAreYouExperiencingIssuesWithThisSiteDesc`, {
          $1: content => <a href="#" onClick={handleLearnMoreClick} style={{ color: 'var(--leo-color-text-secondary)' }}>{content}</a>
        })}
      </div>
      <div
        style={{
          paddingTop: 'var(--leo-spacing-xl)',
          display: 'flex',
          gap: 'var(--leo-spacing-m)'
        }}
      >
        <Button kind="outline" size="medium" onClick={onDismissRepeatedReloadsAdBlockOnlyModePromptClick}>
          {getLocale('braveShieldsDismissAlert')}
        </Button>
        <Button size="medium" onClick={() => onEnableAdBlockOnlyModeClick(/* enableShields: */ false)}>
          {getLocale('braveShieldsEnableAdBlockOnlyMode')}
        </Button>
      </div>
    </div>
  );
}

export function AdBlockOnlyModePromptAfterShieldsDisabled({ onDismiss }: { onDismiss: () => void }) {
  return (
    <div style={{ padding: '0 var(--leo-spacing-xl) var(--leo-spacing-xl)' }}>
      <Alert
        type='info'
        hideIcon
        hasActions={false}
      >
        <div style={{ font: 'var(--leo-font-heading-h4)' }}>
          {getLocale('braveShieldsAreYouExperiencingIssuesWithShieldsTitle')}
        </div>
        <div style={{ font: 'var(--leo-font-default-regular)' }}>
          {formatLocale(`braveShieldsAreYouExperiencingIssuesWithShieldsDesc`, {
            $1: content => <a href="#" onClick={handleLearnMoreClick}>{content}</a>
          })}
        </div>
        <div
          style={{
            paddingTop: 'var(--leo-spacing-xl)',
            display: 'flex',
            gap: 'var(--leo-spacing-m)'
          }}
        >
          <Button size="medium" onClick={() => onEnableAdBlockOnlyModeClick(/* enableShields: */ true)}>
            {getLocale('braveShieldsEnableAdBlockOnlyMode')}
          </Button>
          <Button kind="plain" size="medium" onClick={() => onDismissShieldsDisabledAdBlockOnlyModePromptClick(onDismiss)}>
            {getLocale('braveShieldsDismissAlert')}
          </Button>
        </div>
      </Alert>
    </div>
  )
}
