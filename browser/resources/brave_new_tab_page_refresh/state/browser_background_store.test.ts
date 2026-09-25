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
  () => ({ SponsoredRichMediaAdEventHandler: { getRemote: () => ({}) } }),
)

function createMockNewTabPageProxy() {
  let registeredListeners: Partial<NewTabPageInterface> = {}
  return {
    handler: {
      getBackgroundsEnabled: jest.fn().mockResolvedValue({ enabled: true }),
      getSponsoredImagesEnabled: jest.fn().mockResolvedValue({ enabled: true }),
      getBraveBackgrounds: jest.fn().mockResolvedValue({ backgrounds: [] }),
      getSelectedBackground: jest.fn().mockResolvedValue({ background: null }),
      getCustomBackgrounds: jest.fn().mockResolvedValue({ backgrounds: [] }),
      getSponsoredImageBackground: jest
        .fn()
        .mockResolvedValue({ background: null }),
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

  async function simulateSponsoredImagesEnabledUpdate(enabled: boolean) {
    newTabPageProxy.handler.getSponsoredImagesEnabled.mockResolvedValue({
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

  it('should update sponsoredImagesEnabled to false on a backgrounds update', async () => {
    await simulateSponsoredImagesEnabledUpdate(false)

    expect(store.getState().sponsoredImagesEnabled).toBe(false)
  })

  it('should update sponsoredImagesEnabled to true on a backgrounds update', async () => {
    // Set to disabled state, since sponsoredImagesEnabled defaults to true.
    await simulateSponsoredImagesEnabledUpdate(false)
    expect(store.getState().sponsoredImagesEnabled).toBe(false)

    await simulateSponsoredImagesEnabledUpdate(true)

    expect(store.getState().sponsoredImagesEnabled).toBe(true)
  })
})
