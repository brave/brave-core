/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { waitFor } from '@testing-library/react'

import {
  createTransferApi,
  inboxPageSize,
  TransferApiInit,
  TransferState,
} from './transfer_api'
import {
  createMockDevices,
  createMockOffers,
  createMockSettings,
  createMockTransfers,
} from './transfer_api_mock'

type Page = Parameters<TransferApiInit['bindPage']>[0]

function createApiWithFakeHandler() {
  let page: Page | undefined
  const pageHandler = {
    getServiceState: jest.fn(async () => ({
      state: { syncEnabled: true, discoveryActive: true },
    })),
    getDevices: jest.fn(async () => ({ devices: [] })),
    getTransfers: jest.fn(async () => ({ transfers: [] })),
    getPendingOffers: jest.fn(async () => ({ offers: [] })),
    getSettings: jest.fn(async () => ({ settings: createMockSettings() })),
    getInbox: jest.fn(async () => ({ items: [], hasMore: false })),
    respondToOffer: jest.fn(async () => ({ success: true })),
    clearHistory: jest.fn(async () => ({ success: true })),
    getPermissions: jest.fn(),
  }
  const api = createTransferApi({
    pageHandler: pageHandler as unknown as TransferApiInit['pageHandler'],
    bindPage: (p) => {
      page = p
    },
    isIOS: false,
    isMobile: false,
  })
  return { api, page: page!, pageHandler }
}

describe('createTransferApi', () => {
  it('binds the page before sending requests', () => {
    const { page, pageHandler } = createApiWithFakeHandler()
    expect(page).toBeDefined()
    expect(pageHandler.getDevices).toHaveBeenCalled()
  })

  it('does not request iOS permissions on desktop', () => {
    const { pageHandler } = createApiWithFakeHandler()
    expect(pageHandler.getPermissions).not.toHaveBeenCalled()
  })

  it('applies device, offer and transfer events from the host', async () => {
    const { api, page } = createApiWithFakeHandler()
    await waitFor(() => expect(api.getDevices.current()).toEqual([]))

    page.onDevicesChanged(createMockDevices())
    expect(api.getDevices.current()).toHaveLength(4)

    const [offer] = createMockOffers()
    page.onOfferReceived(offer)
    page.onOfferReceived(offer)
    expect(api.getPendingOffers.current()).toEqual([offer])
    page.onOfferRemoved(offer.id)
    expect(api.getPendingOffers.current()).toEqual([])

    const [transfer] = createMockTransfers()
    page.onTransferUpdated(transfer)
    page.onTransferUpdated({ ...transfer, state: TransferState.kCompleted })
    expect(api.getTransfers.current()).toEqual([
      { ...transfer, state: TransferState.kCompleted },
    ])
  })

  it('removes an offer as soon as it is answered', async () => {
    const { api, page, pageHandler } = createApiWithFakeHandler()
    const [offer] = createMockOffers()
    page.onOfferReceived(offer)

    await api.respondToOffer([offer.id, true])
    expect(pageHandler.respondToOffer).toHaveBeenCalledWith(offer.id, true)
    expect(api.getPendingOffers.current()).toEqual([])
  })

  it('pages the inbox and resets paging when history is cleared', async () => {
    const { api, pageHandler } = createApiWithFakeHandler()
    expect(api.inboxItemCount.current()).toBe(inboxPageSize)

    api.showOlderInboxItems()
    expect(api.inboxItemCount.current()).toBe(2 * inboxPageSize)

    await api.clearHistory()
    expect(pageHandler.clearHistory).toHaveBeenCalled()
    expect(api.inboxItemCount.current()).toBe(inboxPageSize)
  })
})
