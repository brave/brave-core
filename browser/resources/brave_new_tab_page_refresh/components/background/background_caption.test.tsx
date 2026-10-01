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
  SponsoredImageBackground,
  gradientPreviewBackground,
} from '../../state/background_store'
import { BackgroundCaption } from './background_caption'

function createSponsoredBackground(
  overrides: Partial<SponsoredImageBackground> = {},
): SponsoredImageBackground {
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
    sponsoredImagesEnabled: true,
    braveBackgrounds: [],
    customBackgrounds: [],
    selectedBackground: {
      type: SelectedBackgroundType.kGradient,
      value: gradientPreviewBackground,
    },
    backgroundRotateIndex: 0,
    backgroundRandomValue: 0,
    sponsoredImageBackground: null,
    sponsoredRichMediaBaseUrl: '',
    actions: {
      setBackgroundsEnabled() {},
      setSponsoredImagesEnabled() {},
      selectBackground() {},
      async showCustomBackgroundChooser() {
        return false
      },
      async removeCustomBackground() {},
      notifySponsoredImageLoadError() {},
      notifySponsoredImageLogoClicked() {},
      notifySponsoredRichMediaEvent() {},
      notifyNewTabTakeoverDisclosureLearnMoreClicked() {},
      notifyNewTabTakeoverDisclosureTooltipAutoDisplayed() {},
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
  it('should show the disclosure for a sponsored image background', () => {
    renderCaption({ sponsoredImageBackground: createSponsoredBackground() })
    expect(queryDisclosureLabel()).toBeInTheDocument()
  })

  it('should show the disclosure for a sponsored rich media background', () => {
    renderCaption({
      sponsoredImageBackground: createSponsoredBackground({
        wallpaperType: 'richMedia',
      }),
    })
    expect(queryDisclosureLabel()).toBeInTheDocument()
  })

  it('should not show the disclosure when there is no sponsored background', () => {
    renderCaption({ sponsoredImageBackground: null })
    expect(queryDisclosureLabel()).not.toBeInTheDocument()
  })

  it('should not show the disclosure when NTP backgrounds are disabled', () => {
    renderCaption({
      backgroundsEnabled: false,
      sponsoredImageBackground: createSponsoredBackground(),
    })
    expect(queryDisclosureLabel()).not.toBeInTheDocument()
  })
})
