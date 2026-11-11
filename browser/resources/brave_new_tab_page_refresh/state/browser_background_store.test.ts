/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createBackgroundStore } from './browser_background_store'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { NewTabPageInterface } from 'gen/brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.m.js'

jest.mock('./new_tab_page_proxy')

jest.mock(
  'gen/brave/components/ntp_background_images/browser/mojom/'
    + 'ntp_background_images.mojom.m.js',
  () => ({ SponsoredContentAdEventHandler: { getRemote: () => ({}) } }),
)

function createMockNewTabPageProxy() {
  let registeredListeners: Partial<NewTabPageInterface> = {}
  return {
    handler: {
      getBackgroundsEnabled: jest.fn().mockResolvedValue({ enabled: true }),
      getNewTabTakeoversEnabled: jest.fn().mockResolvedValue({ enabled: true }),
      getBraveBackgrounds: jest.fn().mockResolvedValue({ backgrounds: [] }),
      getSelectedBackground: jest.fn().mockResolvedValue({ background: null }),
      getCustomBackgrounds: jest.fn().mockResolvedValue({ backgrounds: [] }),
      getNewTabTakeoverBackground: jest
        .fn()
        .mockResolvedValue({ background: null }),
      notifyNewTabTakeoverDisclosureLearnMoreClicked: jest.fn(),
      notifyNewTabTakeoverDisclosureTooltipAutoDisplayed: jest.fn(),
    },
    addListeners: jest.fn((newListeners: Partial<NewTabPageInterface>) => {
      registeredListeners = newListeners
      return () => {}
    }),
    get listeners() {
      return registeredListeners
    },
  }
}

describe('createBackgroundStore', () => {
  let newTabPageProxy: ReturnType<typeof createMockNewTabPageProxy>
  let store: ReturnType<typeof createBackgroundStore>

  async function simulateNewTabTakeoversEnabledUpdate(enabled: boolean) {
    newTabPageProxy.handler.getNewTabTakeoversEnabled.mockResolvedValue({
      enabled,
    })
    newTabPageProxy.listeners.onBackgroundsUpdated()
    await jest.runOnlyPendingTimersAsync()
  }

  beforeEach(async () => {
    jest.useFakeTimers()

    newTabPageProxy = createMockNewTabPageProxy()
    const getInstance = NewTabPageProxy.getInstance as jest.Mock
    getInstance.mockReturnValue(newTabPageProxy)

    store = createBackgroundStore()
    await jest.runOnlyPendingTimersAsync()
  })

  afterEach(() => {
    jest.useRealTimers()
  })

  it('should update newTabTakeoversEnabled to false on a backgrounds update', async () => {
    await simulateNewTabTakeoversEnabledUpdate(false)

    expect(store.getState().newTabTakeoversEnabled).toBe(false)
  })

  it('should update newTabTakeoversEnabled to true on a backgrounds update', async () => {
    // Set to disabled state, since newTabTakeoversEnabled defaults to true.
    await simulateNewTabTakeoversEnabledUpdate(false)
    expect(store.getState().newTabTakeoversEnabled).toBe(false)

    await simulateNewTabTakeoversEnabledUpdate(true)

    expect(store.getState().newTabTakeoversEnabled).toBe(true)
  })

  describe('new tab takeover disclosure', () => {
    beforeEach(async () => {
      newTabPageProxy.handler.getNewTabTakeoverBackground.mockResolvedValue({
        background: { shouldAutoDisplayNewTabTakeoverDisclosure: true },
      })
      store = createBackgroundStore()
      await jest.runOnlyPendingTimersAsync()
    })

    it('should stop auto-displaying the disclosure after learn more is clicked', () => {
      store.getState().actions.notifyNewTabTakeoverDisclosureLearnMoreClicked()

      expect(
        newTabPageProxy.handler.notifyNewTabTakeoverDisclosureLearnMoreClicked,
      ).toHaveBeenCalledTimes(1)
      expect(
        store.getState().newTabTakeoverBackground
          ?.shouldAutoDisplayNewTabTakeoverDisclosure,
      ).toBe(false)
    })

    it('should stop auto-displaying the disclosure for the current tab after it is auto-displayed', () => {
      store.getState().actions.notifyNewTabTakeoverDisclosureAutoDisplayed()

      expect(
        newTabPageProxy.handler
          .notifyNewTabTakeoverDisclosureTooltipAutoDisplayed,
      ).toHaveBeenCalledTimes(1)
      expect(
        store.getState().newTabTakeoverBackground
          ?.shouldAutoDisplayNewTabTakeoverDisclosure,
      ).toBe(false)
    })
  })
})
