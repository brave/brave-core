/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  Device,
  DevicePolicy,
  DeviceType,
  FileData,
  InboxItem,
  IncomingOffer,
  ItemKind,
  PageHandlerFactory,
  PageHandlerInterface,
  PageHandlerRemote,
  PageInterface,
  PageReceiver,
  PermissionStatus,
  Permissions,
  ReceiveMode,
  SendResult,
  ServiceState,
  Settings,
  Transfer,
  TransferDirection,
  TransferItem,
  TransferState,
  TrustSource,
} from 'gen/brave/components/brave_transfer/common/brave_transfer.mojom.m.js'

import { createInterfaceApi, endpointsFor, state } from '$web-common/api'
import { loadTimeData } from '$web-common/loadTimeData'

export {
  Device,
  DevicePolicy,
  DeviceType,
  FileData,
  InboxItem,
  IncomingOffer,
  ItemKind,
  PermissionStatus,
  Permissions,
  ReceiveMode,
  SendResult,
  ServiceState,
  Settings,
  Transfer,
  TransferDirection,
  TransferItem,
  TransferState,
  TrustSource,
}

// The inbox shows the newest items first, and loads older items in pages of
// this size.
export const inboxPageSize = 100

export interface TransferApiInit {
  pageHandler: PageHandlerInterface
  bindPage: (page: PageInterface) => void
  isIOS: boolean
  isMobile: boolean
}

function defaultInit(): TransferApiInit {
  const pageHandler = new PageHandlerRemote()
  return {
    pageHandler,
    bindPage(page) {
      PageHandlerFactory.getRemote().createPageHandler(
        new PageReceiver(page).$.bindNewPipeAndPassRemote(),
        pageHandler.$.bindNewPipeAndPassReceiver(),
      )
    },
    isIOS: loadTimeData.getBoolean('isIOS'),
    isMobile: loadTimeData.getBoolean('isMobile'),
  }
}

function upsertById<T extends { id: string }>(list: T[], item: T) {
  const index = list.findIndex((existing) => existing.id === item.id)
  if (index === -1) {
    return [item, ...list]
  }
  const next = [...list]
  next[index] = item
  return next
}

export function createTransferApi(init: TransferApiInit = defaultInit()) {
  const { pageHandler } = init

  // The page must be bound before any request is sent, because the handler
  // remote only becomes usable once the factory has been called.
  init.bindPage({
    onServiceStateChanged(serviceState) {
      api.getServiceState.update(serviceState)
    },
    onDevicesChanged(devices) {
      api.getDevices.update(devices)
    },
    onOfferReceived(offer) {
      api.getPendingOffers.update((offers) => upsertById(offers ?? [], offer))
    },
    onOfferRemoved(offerId) {
      api.getPendingOffers.update((offers) =>
        (offers ?? []).filter((offer) => offer.id !== offerId),
      )
    },
    onTransferUpdated(transfer) {
      api.getTransfers.update((transfers) =>
        upsertById(transfers ?? [], transfer),
      )
    },
    onInboxChanged() {
      api.getInbox.invalidate(0, api.inboxItemCount.current())
    },
    onSettingsChanged(settings) {
      api.getSettings.update(settings)
    },
    onPermissionsChanged(permissions) {
      api.getPermissions.update(permissions)
    },
  })

  const api = createInterfaceApi({
    endpoints: {
      ...endpointsFor(pageHandler, {
        getServiceState: {
          response: (r) => r.state,
          prefetchWithArgs: [],
          placeholderData: {
            syncEnabled: true,
            discoveryActive: false,
          } as ServiceState,
        },
        getDevices: {
          response: (r) => r.devices,
          prefetchWithArgs: [],
          placeholderData: [] as Device[],
        },
        setDevicePolicy: {
          mutationResponse: (r) => r.success,
          onMutate: ([deviceId, policy]: [string, DevicePolicy]) => {
            api.getDevices.update((devices) =>
              (devices ?? []).map((device) =>
                device.id === deviceId ? { ...device, policy } : device,
              ),
            )
          },
        },
        removeDevice: {
          mutationResponse: (r) => r.success,
          onMutate: ([deviceId]: [string]) => {
            api.getDevices.update((devices) =>
              (devices ?? []).filter((device) => device.id !== deviceId),
            )
          },
        },
        sendText: {
          mutationResponse: (r) => r.result,
        },
        sendFiles: {
          mutationResponse: (r) => r.result,
        },
        pickAndSend: {
          mutationResponse: (r) => r.result,
        },
        getTransfers: {
          response: (r) => r.transfers,
          prefetchWithArgs: [],
          placeholderData: [] as Transfer[],
        },
        getPendingOffers: {
          response: (r) => r.offers,
          prefetchWithArgs: [],
          placeholderData: [] as IncomingOffer[],
        },
        respondToOffer: {
          mutationResponse: (r) => r.success,
          onMutate: ([offerId]: [string, boolean]) => {
            api.getPendingOffers.update((offers) =>
              (offers ?? []).filter((offer) => offer.id !== offerId),
            )
          },
        },
        getInbox: {
          response: (r) => ({ items: r.items, hasMore: r.hasMore }),
        },
        openItem: {
          mutationResponse: (r) => r.success,
        },
        showItemInFolder: {
          mutationResponse: (r) => r.success,
        },
        openLink: {
          mutationResponse: (r) => r.success,
        },
        clearHistory: {
          mutationResponse: (r) => r.success,
          onSuccess: () => {
            api.inboxItemCount.update(inboxPageSize)
            api.getInbox.invalidate(0, inboxPageSize)
          },
        },
        getSettings: {
          response: (r) => r.settings,
          prefetchWithArgs: [],
        },
        updateSettings: {
          mutationResponse: (r) => r.success,
          onMutate: ([settings]: [Settings]) => {
            api.getSettings.update(settings)
          },
        },
        chooseSaveDirectory: {
          mutationResponse: (r) => r.changed,
          onSuccess: (changed) => {
            if (changed) {
              api.getSettings.invalidate()
            }
          },
        },
        getPermissions: {
          response: (r) => r.permissions,
          enabled: init.isIOS,
        },
        requestPermissions: {
          mutationResponse: (r) => r.permissions,
          onSuccess: (permissions) => {
            api.getPermissions.update(permissions)
          },
        },
      }),
      inboxItemCount: state<number>(inboxPageSize),
      isIOS: state(init.isIOS),
      isMobile: state(init.isMobile),
    },

    actions: {
      openSyncSettings: () => pageHandler.openSyncSettings(),
      cancelTransfer: (transferId: string) =>
        pageHandler.cancelTransfer(transferId),
      openSystemSettings: () => pageHandler.openSystemSettings(),
      showOlderInboxItems: () => {
        api.inboxItemCount.update(api.inboxItemCount.current() + inboxPageSize)
      },
    },
  })

  return api
}

export type TransferApi = ReturnType<typeof createTransferApi>
