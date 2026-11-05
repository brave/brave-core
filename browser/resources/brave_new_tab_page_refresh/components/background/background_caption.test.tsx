/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import { createStateStore } from '$web-common/state_store'
import { BackgroundContext } from '../../context/background_context'
import {
  BackgroundState,
  NewTabPageAdMetricType,
  SelectedBackgroundType,
  NewTabTakeoverBackground,
  gradientPreviewBackground,
} from '../../state/background_store'
import { BackgroundCaption } from './background_caption'

function createNewTabTakeoverBackground(
  overrides: Partial<NewTabTakeoverBackground> = {},
): NewTabTakeoverBackground {
  return {
    wallpaperType: '',
    wallpaperId: 'wallpaper-1',
    creativeInstanceId: '',
    campaignId: '',
    imageUrl: '',
    logo: undefined,
    metricType: NewTabPageAdMetricType.kConfirmation,
    shouldAutoDisplayNewTabTakeoverDisclosure: false,
    ...overrides,
  }
}

function renderCaption(overrides: Partial<BackgroundState> = {}) {
  const store = createStateStore<BackgroundState>({
    initialized: true,
    backgroundsEnabled: true,
    backgroundsCustomizable: true,
    newTabTakeoversEnabled: true,
    braveBackgrounds: [],
    customBackgrounds: [],
    selectedBackground: {
      type: SelectedBackgroundType.kGradient,
      value: gradientPreviewBackground,
    },
    backgroundRotateIndex: 0,
    backgroundRandomValue: 0,
    newTabTakeoverBackground: null,
    sponsoredRichMediaBaseUrl: '',
    actions: {
      setBackgroundsEnabled() {},
      setNewTabTakeoversEnabled() {},
      selectBackground() {},
      async showCustomBackgroundChooser() {
        return false
      },
      async removeCustomBackground() {},
      notifyNewTabTakeoverLoadError() {},
      notifyNewTabTakeoverLogoClicked() {},
      notifyNewTabTakeoverRichMediaEvent() {},
      notifyNewTabTakeoverDisclosureLearnMoreClicked() {},
      notifyNewTabTakeoverDisclosureAutoDisplayed() {},
    },
    ...overrides,
  })
  return render(
    <BackgroundContext.Provider value={store}>
      <BackgroundCaption />
    </BackgroundContext.Provider>,
  )
}

function queryDisclosureLabel() {
  return screen.queryByText('NEW_TAB_TAKEOVER_DISCLOSURE_LABEL')
}

describe('BackgroundCaption', () => {
  it('should show the disclosure for sponsored image background', () => {
    renderCaption({
      newTabTakeoverBackground: createNewTabTakeoverBackground(),
    })
    expect(queryDisclosureLabel()).toBeInTheDocument()
  })

  it('should show the disclosure for sponsored rich media background', () => {
    renderCaption({
      newTabTakeoverBackground: createNewTabTakeoverBackground({
        wallpaperType: 'richMedia',
      }),
    })
    expect(queryDisclosureLabel()).toBeInTheDocument()
  })

  it('should not show the disclosure when there is no sponsored background', () => {
    renderCaption({ newTabTakeoverBackground: null })
    expect(queryDisclosureLabel()).not.toBeInTheDocument()
  })

  it('should not show the disclosure when NTP backgrounds are disabled', () => {
    renderCaption({
      backgroundsEnabled: false,
      newTabTakeoverBackground: createNewTabTakeoverBackground(),
    })
    expect(queryDisclosureLabel()).not.toBeInTheDocument()
  })
})
