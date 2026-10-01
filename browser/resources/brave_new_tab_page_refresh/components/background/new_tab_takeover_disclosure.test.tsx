/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { act, fireEvent, render, screen } from '@testing-library/react'
import { createStateStore } from '$web-common/state_store'
import { BackgroundContext } from '../../context/background_context'
import {
  BackgroundState,
  NewTabPageAdMetricType,
  SelectedBackgroundType,
  SponsoredImageBackground,
  gradientPreviewBackground,
} from '../../state/background_store'
import { NewTabTakeoverDisclosure } from './new_tab_takeover_disclosure'

// The default $web-common/locale mock (see components/test/testSetup.ts)
// echoes the string key back verbatim, which has no $1 placeholder for
// formatString to replace. Override just the tooltip text with a string that
// has one, so the tooltip renders as it would in production, while the label
// itself keeps echoing its key so it stays queryable by name.
jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    if (key === 'NEW_TAB_TAKEOVER_DISCLOSURE_TOOLTIP_TEXT') {
      return 'Sponsored Ads. $1Learn more/$1.'
    }
    return key
  },
}))

function getTooltipElement() {
  return document.querySelector('leo-tooltip') as
    | (HTMLElement & { visible?: boolean })
    | null
}

function getLabel() {
  return screen.getByText('NEW_TAB_TAKEOVER_DISCLOSURE_LABEL')
}

function createBackground(
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
    shouldAutoShowNewTabTakeoverDisclosure: false,
    ...overrides,
  }
}

function createBackgroundStore(
  actions: Partial<BackgroundState['actions']> = {},
) {
  return createStateStore<BackgroundState>({
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
      ...actions,
    },
  })
}

function renderDisclosure(
  background: SponsoredImageBackground,
  actions: Partial<BackgroundState['actions']> = {},
) {
  const store = createBackgroundStore(actions)
  return render(
    <BackgroundContext.Provider value={store}>
      <NewTabTakeoverDisclosure
        background={background}
        placement='top'
      />
    </BackgroundContext.Provider>,
  )
}

describe('NewTabTakeoverDisclosure', () => {
  it('should render the label', () => {
    renderDisclosure(createBackground())
    expect(getLabel()).toBeInTheDocument()
  })

  describe('tooltip visibility', () => {
    beforeEach(() => {
      jest.useFakeTimers()
    })

    afterEach(() => {
      jest.useRealTimers()
    })

    it('should not auto-show the tooltip when the flag is false', () => {
      renderDisclosure(
        createBackground({ shouldAutoShowNewTabTakeoverDisclosure: false }),
      )
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(false)
    })

    it('should auto-show the tooltip after a delay when the flag is true', () => {
      renderDisclosure(
        createBackground({ shouldAutoShowNewTabTakeoverDisclosure: true }),
      )
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(true)
    })

    it('should still auto-show the tooltip if the pointer enters and leaves the label during the delay', () => {
      // Pins the requirement that auto-show uses its own timer rather than
      // the hover timer: `onMouseLeave` clears the hover timer, and sharing
      // it would let a passing pointer cancel a display the browser has
      // already counted against the auto-show cap.
      renderDisclosure(
        createBackground({ shouldAutoShowNewTabTakeoverDisclosure: true }),
      )
      fireEvent.mouseEnter(getLabel())
      act(() => {
        jest.advanceTimersByTime(300)
      })
      fireEvent.mouseLeave(getLabel())
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(true)
    })

    it('should not show the tooltip immediately on hover', () => {
      renderDisclosure(createBackground())
      fireEvent.mouseEnter(getLabel())
      expect(getTooltipElement()?.visible).toBe(false)
    })

    it('should show the tooltip after a delay on hover', () => {
      renderDisclosure(createBackground())
      fireEvent.mouseEnter(getLabel())
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(true)
    })

    it('should not show the tooltip if the pointer leaves before the hover delay elapses', () => {
      renderDisclosure(createBackground())
      fireEvent.mouseEnter(getLabel())
      act(() => {
        jest.advanceTimersByTime(300)
      })
      fireEvent.mouseLeave(getLabel())
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(false)
    })

    it('should show the tooltip immediately on focus', () => {
      renderDisclosure(createBackground())
      fireEvent.focus(getLabel())
      expect(getTooltipElement()?.visible).toBe(true)
    })
  })

  it('should notify the browser when the learn-more link is clicked', () => {
    const notifyNewTabTakeoverDisclosureLearnMoreClicked = jest.fn()
    renderDisclosure(createBackground(), {
      notifyNewTabTakeoverDisclosureLearnMoreClicked,
    })
    act(() => getLabel().focus())
    fireEvent.click(screen.getByRole('link', { name: 'Learn more' }))
    expect(
      notifyNewTabTakeoverDisclosureLearnMoreClicked,
    ).toHaveBeenCalledTimes(1)
  })
})
