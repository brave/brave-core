/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import DropDown from '@brave/leo/react/dropdown'
import Toggle from '@brave/leo/react/toggle'
import { color, icon, spacing } from '@brave/leo/tokens/css/variables'
import { getLocale } from '$web-common/locale'
import { loadTimeData } from '$web-common/loadTimeData'
import styled from 'styled-components'

import { useBraveNews } from '../shared/Context'
import OpmlControls from './OpmlControls'
import { SettingsPanel } from './SettingsPanel'

const Container = styled.div`
  display: flex;
  flex-direction: column;
`

const ControlRow = styled.div`
  padding: ${spacing.l} ${spacing['2Xl']};
  border-bottom: solid 1px ${color.divider.subtle};

  display: flex;
  align-items: center;
  gap: ${spacing.m};

  label {
    flex: 1 1 auto;
  }

  &:last-child {
    border-bottom: none;
  }
`

const EnableRow = styled(Toggle)`
  padding: ${spacing.l} ${spacing['2Xl']};
  border-bottom: solid 1px ${color.divider.subtle};

  --leo-toggle-label-flex-direction: row-reverse;
  --leo-toggle-label-gap: ${spacing.xl};

  .label {
    --leo-icon-size: ${icon.m};

    flex: 1 1 auto;
    display: flex;
    align-items: center;
    gap: ${spacing.xl};
    color: ${color.text.primary};
  }

  &:last-child {
    border-bottom: none;
  }
`

export default function NewsSettings() {
  const {
    isShowOnNTPPrefEnabled,
    toggleBraveNewsOnNTP,
    openArticlesInNewTab,
    setOpenArticlesInNewTab,
  } = useBraveNews()

  const feedV2Enabled = loadTimeData.getBoolean(
    'featureFlagBraveNewsFeedV2Enabled',
  )
  const controlsDisabled = !isShowOnNTPPrefEnabled

  return (
    <SettingsPanel title={getLocale(S.BRAVE_NEWS_SETTINGS_SECTION_TITLE)}>
      <Container>
        <EnableRow
          size='small'
          checked={isShowOnNTPPrefEnabled}
          onChange={({ checked }) => {
            toggleBraveNewsOnNTP(checked)
          }}
        >
          <span className='label'>
            {getLocale(S.BRAVE_NEWS_ENABLE_LABEL)}
          </span>
        </EnableRow>
        {feedV2Enabled && (
          <ControlRow>
            <label>{getLocale(S.BRAVE_NEWS_OPEN_ARTICLES_IN)}</label>
            <DropDown
              value={openArticlesInNewTab ? 'true' : 'false'}
              positionStrategy='fixed'
              disabled={controlsDisabled}
              onChange={(detail) => {
                setOpenArticlesInNewTab(detail.value === 'true')
              }}
            >
              <span slot='value'>
                {openArticlesInNewTab
                  ? getLocale(S.BRAVE_NEWS_OPEN_ARTICLES_IN_NEW_TAB)
                  : getLocale(S.BRAVE_NEWS_OPEN_ARTICLES_IN_CURRENT_TAB)}
              </span>
              <leo-option value='true'>
                {getLocale(S.BRAVE_NEWS_OPEN_ARTICLES_IN_NEW_TAB)}
              </leo-option>
              <leo-option value='false'>
                {getLocale(S.BRAVE_NEWS_OPEN_ARTICLES_IN_CURRENT_TAB)}
              </leo-option>
            </DropDown>
          </ControlRow>
        )}
        <ControlRow>
          <OpmlControls disabled={controlsDisabled} />
        </ControlRow>
      </Container>
    </SettingsPanel>
  )
}
