/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import { createStateStore } from '$web-common/state_store'
import { BackgroundContext } from '../../context/background_context'
import { RewardsContext } from '../../context/rewards_context'
import {
  BackgroundState,
  SelectedBackgroundType,
  gradientPreviewBackground,
} from '../../state/background_store'
import { RewardsState } from '../../state/rewards_store'
import { BackgroundPanel } from './background_panel'

// The default $web-common/locale mock (see components/test/testSetup.ts)
// echoes the string key back verbatim, which has no $1/$2 placeholders for
// formatString to replace. Override just the New Tab Takeovers earning text
// so it renders with its links as it would in production.
jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    if (key === 'NEW_TAB_SHOW_SPONSORED_IMAGES_EARNING_TEXT') {
      return 'Earn by viewing New Tab Takeovers. $1Settings/$1 $2Learn more/$2.'
    }
    return key
  },
}))

function createBackgroundStore(
  state: Partial<BackgroundState>,
  actions: Partial<BackgroundState['actions']> = {},
) {
  return createStateStore<BackgroundState>({
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
      ...actions,
    },
    ...state,
  })
}

function createRewardsStore(state: Partial<RewardsState>) {
  return createStateStore<RewardsState>({
    initialized: true,
    rewardsFeatureEnabled: true,
    showRewardsWidget: false,
    rewardsEnabled: false,
    rewardsExternalWallet: null,
    rewardsBalance: null,
    rewardsExchangeRate: 0,
    rewardsAdsViewed: null,
    minEarningsPreviousMonth: 0,
    payoutStatus: {},
    tosUpdateRequired: false,
    actions: {
      setShowRewardsWidget() {},
      recordNewTabOnboardingClick() {},
    },
    ...state,
  })
}

function renderPanel(
  backgroundState: Partial<BackgroundState> = {},
  rewardsState: Partial<RewardsState> = {},
  actions: Partial<BackgroundState['actions']> = {},
) {
  const backgroundStore = createBackgroundStore(backgroundState, actions)
  const rewardsStore = createRewardsStore(rewardsState)
  render(
    <BackgroundContext.Provider value={backgroundStore}>
      <RewardsContext.Provider value={rewardsStore}>
        <BackgroundPanel />
      </RewardsContext.Provider>
    </BackgroundContext.Provider>,
  )
}

describe('BackgroundPanel', () => {
  it('should render the New Tab Takeovers description with a learn more link', () => {
    renderPanel({}, { rewardsFeatureEnabled: true, rewardsEnabled: false })
    expect(
      screen.getByText('Earn by viewing New Tab Takeovers.', {
        exact: false,
      }),
    ).toBeInTheDocument()
    expect(screen.getByRole('link', { name: 'Learn more' })).toHaveAttribute(
      'href',
      expect.stringContaining('support.brave.app'),
    )
  })

  it('should update the New Tab Takeovers setting when its toggle changes', () => {
    const setNewTabTakeoversEnabled = jest.fn()
    renderPanel(
      { newTabTakeoversEnabled: false },
      { rewardsFeatureEnabled: true, rewardsEnabled: false },
      { setNewTabTakeoversEnabled },
    )
    screen.getByText('NEW_TAB_SHOW_SPONSORED_IMAGES_LABEL').click()
    expect(setNewTabTakeoversEnabled).toHaveBeenCalledWith(true)
  })

  it('should stop the learn more link click from reaching the toggle', () => {
    // The Toggle wraps its label content in a native <label>, which
    // forwards an unstopped click to its associated control. Rather than
    // relying on jsdom to replicate that native label activation
    // behavior (uncertain in this test environment), this verifies the
    // fix's actual mechanism directly: the click must not bubble past the
    // description's wrapping element.
    renderPanel({}, { rewardsFeatureEnabled: true, rewardsEnabled: false })
    const onDocumentClick = jest.fn()
    document.addEventListener('click', onDocumentClick)
    try {
      screen.getByRole('link', { name: 'Learn more' }).click()
    } finally {
      document.removeEventListener('click', onDocumentClick)
    }
    expect(onDocumentClick).not.toHaveBeenCalled()
  })
})
