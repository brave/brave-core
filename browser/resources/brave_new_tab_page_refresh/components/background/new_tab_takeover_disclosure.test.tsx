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
    shouldAutoDisplayNewTabTakeoverDisclosure: false,
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
      notifyNewTabTakeoverDisclosureTooltipAutoDisplayed() {},
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
        tooltipPlacement='top-start'
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

    it('should not auto-display the tooltip when the flag is false', () => {
      renderDisclosure(
        createBackground({ shouldAutoDisplayNewTabTakeoverDisclosure: false }),
      )
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(false)
    })

    it('should auto-display the tooltip after a delay when the flag is true', () => {
      renderDisclosure(
        createBackground({ shouldAutoDisplayNewTabTakeoverDisclosure: true }),
      )
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(true)
    })

    it('should still auto-display the tooltip if the pointer enters and leaves the label during the delay', () => {
      // Pins the requirement that auto-display uses its own timer rather than
      // the hover timer: `onMouseLeave` clears the hover timer, and sharing
      // it would let a passing pointer cancel the separately-tracked
      // auto-display timer.
      renderDisclosure(
        createBackground({ shouldAutoDisplayNewTabTakeoverDisclosure: true }),
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
  })

  it('should notify the browser when the learn more link is clicked', () => {
    const notifyNewTabTakeoverDisclosureLearnMoreClicked = jest.fn()
    renderDisclosure(createBackground(), {
      notifyNewTabTakeoverDisclosureLearnMoreClicked,
    })
    fireEvent.click(screen.getByRole('link', { name: 'Learn more' }))
    expect(
      notifyNewTabTakeoverDisclosureLearnMoreClicked,
    ).toHaveBeenCalledTimes(1)
  })

  describe('auto-display notification', () => {
    beforeEach(() => {
      jest.useFakeTimers()
    })

    afterEach(() => {
      jest.useRealTimers()
    })

    it('should notify the browser when the auto-display timer completes', () => {
      const notifyNewTabTakeoverDisclosureTooltipAutoDisplayed = jest.fn()
      renderDisclosure(
        createBackground({ shouldAutoDisplayNewTabTakeoverDisclosure: true }),
        { notifyNewTabTakeoverDisclosureTooltipAutoDisplayed },
      )
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(
        notifyNewTabTakeoverDisclosureTooltipAutoDisplayed,
      ).toHaveBeenCalledTimes(1)
    })

    it('should not notify the browser when the tooltip is shown via hover, not auto-display', () => {
      const notifyNewTabTakeoverDisclosureTooltipAutoDisplayed = jest.fn()
      renderDisclosure(
        createBackground({ shouldAutoDisplayNewTabTakeoverDisclosure: false }),
        { notifyNewTabTakeoverDisclosureTooltipAutoDisplayed },
      )
      fireEvent.mouseEnter(getLabel())
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(true)
      expect(
        notifyNewTabTakeoverDisclosureTooltipAutoDisplayed,
      ).not.toHaveBeenCalled()
    })

    it('should not notify the browser if unmounted before the auto-display delay elapses', () => {
      const notifyNewTabTakeoverDisclosureTooltipAutoDisplayed = jest.fn()
      const { unmount } = renderDisclosure(
        createBackground({ shouldAutoDisplayNewTabTakeoverDisclosure: true }),
        { notifyNewTabTakeoverDisclosureTooltipAutoDisplayed },
      )
      act(() => {
        jest.advanceTimersByTime(300)
      })
      unmount()
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(
        notifyNewTabTakeoverDisclosureTooltipAutoDisplayed,
      ).not.toHaveBeenCalled()
    })

    it('should not notify the browser if the background becomes ineligible before the auto-display delay elapses', () => {
      // Defensive coverage for the auto-display effect's own cleanup. This
      // path can't currently occur in production (the sponsored background
      // is fetched once per page load), but the cleanup is cheap and this
      // pins its behavior: a stale timer must not fire after the background
      // it was scheduled for is no longer eligible.
      const notifyNewTabTakeoverDisclosureTooltipAutoDisplayed = jest.fn()
      const store = createBackgroundStore({
        notifyNewTabTakeoverDisclosureTooltipAutoDisplayed,
      })
      const { rerender } = render(
        <BackgroundContext.Provider value={store}>
          <NewTabTakeoverDisclosure
            background={createBackground({
              shouldAutoDisplayNewTabTakeoverDisclosure: true,
            })}
            tooltipPlacement='top-start'
          />
        </BackgroundContext.Provider>,
      )
      act(() => {
        jest.advanceTimersByTime(300)
      })
      rerender(
        <BackgroundContext.Provider value={store}>
          <NewTabTakeoverDisclosure
            background={createBackground({
              shouldAutoDisplayNewTabTakeoverDisclosure: false,
            })}
            tooltipPlacement='top-start'
          />
        </BackgroundContext.Provider>,
      )
      act(() => {
        jest.advanceTimersByTime(500)
      })
      expect(getTooltipElement()?.visible).toBe(false)
      expect(
        notifyNewTabTakeoverDisclosureTooltipAutoDisplayed,
      ).not.toHaveBeenCalled()
    })
  })
})
