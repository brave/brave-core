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

jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    if (key === 'NEW_TAB_TAKEOVER_DISCLOSURE_TOOLTIP_TEXT') {
      return 'New Tab Takeover Disclosure Tooltip Text. $1Learn more/$1.'
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

function renderNewTabTakeoverDisclosure(
  background: SponsoredImageBackground,
  actions: Partial<BackgroundState['actions']> = {},
) {
  const store = createBackgroundStore(actions)
  return render(
    <BackgroundContext.Provider value={store}>
      <NewTabTakeoverDisclosure background={background} />
    </BackgroundContext.Provider>,
  )
}

describe('NewTabTakeoverDisclosure', () => {
  beforeEach(() => {
    jest.useFakeTimers()
  })

  afterEach(() => {
    jest.useRealTimers()
  })

  it('should render the label', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    expect(getLabel()).toBeInTheDocument()
  })

  it('should notify the browser when the learn more link is clicked', () => {
    const notifyNewTabTakeoverDisclosureLearnMoreClicked = jest.fn()
    renderNewTabTakeoverDisclosure(createBackground(), {
      notifyNewTabTakeoverDisclosureLearnMoreClicked,
    })
    fireEvent.click(screen.getByRole('link', { name: 'Learn more' }))
    expect(
      notifyNewTabTakeoverDisclosureLearnMoreClicked,
    ).toHaveBeenCalledTimes(1)
  })

  it('should not auto-display the tooltip', () => {
    renderNewTabTakeoverDisclosure(
      createBackground({ shouldAutoDisplayNewTabTakeoverDisclosure: false }),
    )
    act(() => {
      jest.advanceTimersByTime(500)
    })
    expect(getTooltipElement()?.visible).toBe(false)
  })

  it('should auto-display the tooltip', () => {
    renderNewTabTakeoverDisclosure(
      createBackground({ shouldAutoDisplayNewTabTakeoverDisclosure: true }),
    )
    act(() => {
      jest.advanceTimersByTime(500)
    })
    expect(getTooltipElement()?.visible).toBe(true)
  })

  it('should still auto-display the tooltip if the pointer enters and leaves the label during the delay', () => {
    renderNewTabTakeoverDisclosure(
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
    renderNewTabTakeoverDisclosure(createBackground())
    fireEvent.mouseEnter(getLabel())
    expect(getTooltipElement()?.visible).toBe(false)
  })

  it('should show the tooltip after a delay on hover', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    fireEvent.mouseEnter(getLabel())
    act(() => {
      jest.advanceTimersByTime(500)
    })
    expect(getTooltipElement()?.visible).toBe(true)
  })

  it('should not show the tooltip if the pointer leaves before the hover delay elapses', () => {
    renderNewTabTakeoverDisclosure(createBackground())
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

  it('should make the label keyboard focusable', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    expect(getLabel()).toHaveAttribute('tabindex', '0')
  })

  it('should show the tooltip on focus', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    fireEvent.focus(getLabel())
    expect(getTooltipElement()?.visible).toBe(true)
  })

  it('should keep the tooltip open when focus moves to the learn more link', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    act(() => getLabel().focus())
    act(() => screen.getByRole('link', { name: 'Learn more' }).focus())
    expect(getTooltipElement()?.visible).toBe(true)
  })

  it('should close the tooltip when focus moves away from it entirely', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    act(() => getLabel().focus())
    const elsewhere = document.createElement('button')
    document.body.appendChild(elsewhere)
    act(() => elsewhere.focus())
    expect(getTooltipElement()?.visible).toBe(false)
    elsewhere.remove()
  })

  it('should show the tooltip when the label is clicked', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    fireEvent.click(getLabel())
    expect(getTooltipElement()?.visible).toBe(true)
  })

  it('should show the tooltip when the learn more link receives focus', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    act(() => screen.getByRole('link', { name: 'Learn more' }).focus())
    expect(getTooltipElement()?.visible).toBe(true)
  })

  it('should close the tooltip when the learn more link loses focus', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    act(() => screen.getByRole('link', { name: 'Learn more' }).focus())
    const elsewhere = document.createElement('button')
    document.body.appendChild(elsewhere)
    act(() => elsewhere.focus())
    expect(getTooltipElement()?.visible).toBe(false)
    elsewhere.remove()
  })

  it('should close the tooltip when Escape is pressed', () => {
    renderNewTabTakeoverDisclosure(
      createBackground({ shouldAutoDisplayNewTabTakeoverDisclosure: true }),
    )
    act(() => {
      jest.advanceTimersByTime(500)
    })
    fireEvent.keyDown(document, { key: 'Escape' })
    expect(getTooltipElement()?.visible).toBe(false)
  })

  it('should not auto-display the tooltip after Escape is pressed during the auto-display delay', () => {
    const notifyNewTabTakeoverDisclosureTooltipAutoDisplayed = jest.fn()
    renderNewTabTakeoverDisclosure(
      createBackground({ shouldAutoDisplayNewTabTakeoverDisclosure: true }),
      { notifyNewTabTakeoverDisclosureTooltipAutoDisplayed },
    )
    fireEvent.focus(getLabel())
    act(() => {
      jest.advanceTimersByTime(300)
    })
    fireEvent.keyDown(document, { key: 'Escape' })
    act(() => {
      jest.advanceTimersByTime(500)
    })
    expect(getTooltipElement()?.visible).toBe(false)
    expect(
      notifyNewTabTakeoverDisclosureTooltipAutoDisplayed,
    ).not.toHaveBeenCalled()
  })

  it('should not show the tooltip after Escape is pressed during the hover delay', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    fireEvent.mouseEnter(getLabel())
    fireEvent.focus(getLabel())
    act(() => {
      jest.advanceTimersByTime(300)
    })
    fireEvent.keyDown(document, { key: 'Escape' })
    act(() => {
      jest.advanceTimersByTime(500)
    })
    expect(getTooltipElement()?.visible).toBe(false)
  })

  it('should describe the label with the tooltip content', () => {
    renderNewTabTakeoverDisclosure(createBackground())
    const descriptionId = getLabel().getAttribute('aria-describedby')
    expect(document.getElementById(descriptionId ?? '')).toHaveTextContent(
      'New Tab Takeover Disclosure Tooltip Text. Learn more.',
    )
  })

  it('should notify the browser when the auto-display timer completes', () => {
    const notifyNewTabTakeoverDisclosureTooltipAutoDisplayed = jest.fn()
    renderNewTabTakeoverDisclosure(
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
    renderNewTabTakeoverDisclosure(
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
    const { unmount } = renderNewTabTakeoverDisclosure(
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
})
