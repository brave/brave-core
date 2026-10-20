/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  createTransferApi,
  Device,
  DevicePolicy,
  DeviceType,
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
  TransferApiInit,
  TransferDirection,
  TransferState,
  TrustSource,
} from './transfer_api'

const minute = 60 * 1000
const hour = 60 * minute

// A gradient stands in for decoded photo thumbnails.
const thumbnailUrl =
  'data:image/svg+xml,'
  + encodeURIComponent(
    '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48">'
      + '<defs><linearGradient id="g" x2="1" y2="1">'
      + '<stop offset="0" stop-color="#f6a34a"/>'
      + '<stop offset="1" stop-color="#6b4ccf"/>'
      + '</linearGradient></defs>'
      + '<rect width="48" height="48" fill="url(#g)"/>'
      + '<circle cx="34" cy="14" r="5" fill="#fff8"/>'
      + '<path d="M0 40 16 24l12 12 6-6 14 14v4H0z" fill="#0006"/>'
      + '</svg>',
  )

export function createMockDevices(now = Date.now()): Device[] {
  return [
    {
      id: 'iphone',
      name: "James's iPhone",
      type: DeviceType.kPhone,
      source: TrustSource.kSyncChain,
      isConnected: true,
      lastSeenTime: now,
      keyFingerprint: '3F2A 91C0 7B1E 44D8',
      policy: DevicePolicy.kDefault,
    },
    {
      id: 'ipad',
      name: 'iPad Air',
      type: DeviceType.kTablet,
      source: TrustSource.kSyncChain,
      isConnected: true,
      lastSeenTime: now,
      keyFingerprint: '0C9D 5E12 A6F3 8B70',
      policy: DevicePolicy.kAlwaysAccept,
    },
    {
      id: 'mac-mini',
      name: 'Studio Mac mini',
      type: DeviceType.kDesktop,
      source: TrustSource.kSyncChain,
      isConnected: false,
      lastSeenTime: now - 3 * hour,
      keyFingerprint: '7A41 C2D9 E018 5F6B',
      policy: DevicePolicy.kAlwaysAsk,
    },
    {
      id: 'pixel',
      name: 'Pixel 9',
      type: DeviceType.kPhone,
      source: TrustSource.kSyncChain,
      isConnected: false,
      lastSeenTime: 0,
      keyFingerprint: '',
      policy: DevicePolicy.kBlock,
    },
  ]
}

export function createMockOffers(now = Date.now()): IncomingOffer[] {
  return [
    {
      id: 'offer-1',
      deviceId: 'iphone',
      deviceName: "James's iPhone",
      expiryTime: now + 45 * 1000,
      items: [
        {
          id: 'offer-1-a',
          kind: ItemKind.kImage,
          name: 'IMG_2041.HEIC',
          mimeType: 'image/heic',
          size: BigInt(2_841_600),
          text: '',
        },
        {
          id: 'offer-1-b',
          kind: ItemKind.kFile,
          name: 'Boarding pass.pdf',
          mimeType: 'application/pdf',
          size: BigInt(184_320),
          text: '',
        },
      ],
    },
  ]
}

export function createMockTransfers(): Transfer[] {
  return [
    {
      id: 'transfer-1',
      deviceId: 'ipad',
      deviceName: 'iPad Air',
      direction: TransferDirection.kSend,
      state: TransferState.kInProgress,
      bytesTransferred: BigInt(12_400_000),
      bytesTotal: BigInt(31_000_000),
      items: [
        {
          id: 'transfer-1-a',
          kind: ItemKind.kFile,
          name: 'Keynote draft.key',
          mimeType: 'application/octet-stream',
          size: BigInt(31_000_000),
          text: '',
        },
      ],
    },
    {
      id: 'transfer-2',
      deviceId: 'iphone',
      deviceName: "James's iPhone",
      direction: TransferDirection.kSend,
      state: TransferState.kWaitingForResponse,
      bytesTransferred: BigInt(0),
      bytesTotal: BigInt(0),
      items: [
        {
          id: 'transfer-2-a',
          kind: ItemKind.kLink,
          name: '',
          mimeType: '',
          size: BigInt(0),
          text: 'https://brave.com/transfer',
        },
      ],
    },
  ]
}

export function createMockInbox(now = Date.now()): InboxItem[] {
  const base = {
    text: '',
    fileName: '',
    mimeType: '',
    size: BigInt(0),
    thumbnailUrl: '',
    fileMissing: false,
    savedToFallback: false,
  }
  return [
    {
      ...base,
      id: 'inbox-1',
      receivedTime: now - 2 * minute,
      deviceName: "James's iPhone",
      kind: ItemKind.kLink,
      text: 'https://github.com/brave/brave-browser/wiki/Brave-Transfer',
    },
    {
      ...base,
      id: 'inbox-2',
      receivedTime: now - 25 * minute,
      deviceName: "James's iPhone",
      kind: ItemKind.kImage,
      fileName: 'IMG_2038.jpeg',
      mimeType: 'image/jpeg',
      size: BigInt(3_276_800),
      thumbnailUrl,
    },
    {
      ...base,
      id: 'inbox-3',
      receivedTime: now - 1 * hour,
      deviceName: 'iPad Air',
      kind: ItemKind.kText,
      text:
        'Wi-Fi for the offsite: BraveGuest / password is on the whiteboard.\n'
        + 'Parking is on level 2, the badge reader is to the left of the lift.\n'
        + 'Dinner is booked for 7pm at the place across the road, ask for the '
        + 'Brave table. Bring the demo iPhone and the spare charger.',
    },
    {
      ...base,
      id: 'inbox-4',
      receivedTime: now - 26 * hour,
      deviceName: 'iPad Air',
      kind: ItemKind.kFile,
      fileName: 'Q3 roadmap.pdf',
      mimeType: 'application/pdf',
      size: BigInt(1_048_576),
    },
    {
      ...base,
      id: 'inbox-5',
      receivedTime: now - 3 * 24 * hour,
      deviceName: "James's iPhone",
      kind: ItemKind.kFile,
      fileName: 'Screen Recording.mov',
      mimeType: 'video/quicktime',
      size: BigInt(48_234_496),
      fileMissing: true,
    },
  ]
}

export function createMockSettings(): Settings {
  return {
    receiveMode: ReceiveMode.kTrustedDevices,
    autoAcceptTrusted: false,
    autoTrustSyncChain: true,
    notifyOnReceive: true,
    saveDirectory: '',
    defaultSaveDirectory: '/Users/james/Downloads',
  }
}

export interface MockOptions {
  serviceState?: Partial<ServiceState>
  devices?: Device[]
  offers?: IncomingOffer[]
  transfers?: Transfer[]
  inbox?: InboxItem[]
  // Total inbox size reported to the page. Defaults to the length of `inbox`;
  // a larger value makes the mock report more pages.
  inboxTotal?: number
  settings?: Partial<Settings>
  permissions?: Permissions
  isIOS?: boolean
  isMobile?: boolean
  // Called with the method name and arguments of every page handler call,
  // before the mock handles it.
  onHandlerCall?: (method: string, args: unknown[]) => void
}

export function createTransferApiMock(options: MockOptions = {}) {
  let serviceState: ServiceState = {
    syncEnabled: true,
    discoveryActive: true,
    ...options.serviceState,
  }
  let devices = options.devices ?? createMockDevices()
  let offers = options.offers ?? []
  let transfers = options.transfers ?? []
  let inbox = options.inbox ?? []
  const inboxTotal = Math.max(options.inboxTotal ?? 0, inbox.length)
  let settings: Settings = { ...createMockSettings(), ...options.settings }
  let permissions: Permissions = options.permissions ?? {
    localNetwork: PermissionStatus.kGranted,
    photos: PermissionStatus.kLimited,
  }

  let page: Parameters<TransferApiInit['bindPage']>[0]

  let nextId = 1
  const startTransfer = (deviceId: string, items: Transfer['items']) => {
    const device = devices.find((d) => d.id === deviceId)
    if (!device?.isConnected) {
      return SendResult.kDeviceUnavailable
    }
    const total = items.reduce((sum, item) => sum + item.size, BigInt(0))
    const transfer: Transfer = {
      id: `mock-transfer-${nextId++}`,
      deviceId,
      deviceName: device.name,
      direction: TransferDirection.kSend,
      state: TransferState.kWaitingForResponse,
      bytesTransferred: BigInt(0),
      bytesTotal: total,
      items,
    }
    transfers = [transfer, ...transfers]
    page.onTransferUpdated(transfer)
    return SendResult.kSuccess
  }

  const pageHandler: TransferApiInit['pageHandler'] = {
    getServiceState: async () => ({ state: serviceState }),
    openSyncSettings: () => {
      serviceState = { ...serviceState, syncEnabled: true }
      page.onServiceStateChanged(serviceState)
    },
    getDevices: async () => ({ devices }),
    setDevicePolicy: async (deviceId, policy) => {
      devices = devices.map((d) => (d.id === deviceId ? { ...d, policy } : d))
      return { success: true }
    },
    removeDevice: async (deviceId) => {
      devices = devices.filter((d) => d.id !== deviceId)
      return { success: true }
    },
    sendText: async (deviceId, text) => {
      const isLink = /^https?:\/\/\S+$/.test(text.trim())
      return {
        result: startTransfer(deviceId, [
          {
            id: `mock-item-${nextId++}`,
            kind: isLink ? ItemKind.kLink : ItemKind.kText,
            name: '',
            mimeType: '',
            size: BigInt(text.length),
            text,
          },
        ]),
      }
    },
    sendFiles: async (deviceId, files) => ({
      result: startTransfer(
        deviceId,
        files.map((file) => ({
          id: `mock-item-${nextId++}`,
          kind: file.mimeType.startsWith('image/')
            ? ItemKind.kImage
            : ItemKind.kFile,
          name: file.name,
          mimeType: file.mimeType,
          size: BigInt(file.data.length),
          text: '',
        })),
      ),
    }),
    pickAndSend: async (deviceId) => ({
      result: startTransfer(deviceId, [
        {
          id: `mock-item-${nextId++}`,
          kind: ItemKind.kImage,
          name: 'IMG_2042.HEIC',
          mimeType: 'image/heic',
          size: BigInt(2_100_000),
          text: '',
        },
      ]),
    }),
    cancelTransfer: (transferId) => {
      const transfer = transfers.find((t) => t.id === transferId)
      if (transfer) {
        const cancelled = { ...transfer, state: TransferState.kCancelled }
        transfers = transfers.map((t) => (t.id === transferId ? cancelled : t))
        page.onTransferUpdated(cancelled)
      }
    },
    getTransfers: async () => ({ transfers }),
    getPendingOffers: async () => ({ offers }),
    respondToOffer: async (offerId, accept) => {
      const offer = offers.find((o) => o.id === offerId)
      offers = offers.filter((o) => o.id !== offerId)
      if (offer && accept) {
        const received: InboxItem[] = offer.items.map((item) => ({
          id: `inbox-${item.id}`,
          receivedTime: Date.now(),
          deviceName: offer.deviceName,
          kind: item.kind,
          text: item.text,
          fileName: item.name,
          mimeType: item.mimeType,
          size: item.size,
          thumbnailUrl: item.kind === ItemKind.kImage ? thumbnailUrl : '',
          fileMissing: false,
          savedToFallback: false,
        }))
        inbox = [...received, ...inbox]
        page.onInboxChanged()
      }
      return { success: !!offer }
    },
    getInbox: async (offset, count) => {
      const items = inbox.slice(offset, offset + count)
      return {
        items,
        hasMore: offset + count < Math.max(inboxTotal, inbox.length),
      }
    },
    openItem: async () => ({ success: true }),
    showItemInFolder: async () => ({ success: true }),
    openLink: async () => ({ success: true }),
    clearHistory: async () => {
      inbox = []
      return { success: true }
    },
    getSettings: async () => ({ settings }),
    updateSettings: async (next) => {
      settings = next
      return { success: true }
    },
    chooseSaveDirectory: async () => {
      settings = {
        ...settings,
        saveDirectory: '/Users/james/Desktop/Transfers',
      }
      return { changed: true }
    },
    getPermissions: async () => ({ permissions }),
    requestPermissions: async () => {
      permissions = {
        localNetwork: PermissionStatus.kGranted,
        photos: PermissionStatus.kLimited,
      }
      return { permissions }
    },
    openSystemSettings: () => {},
  }

  const { onHandlerCall } = options
  if (onHandlerCall) {
    for (const [method, impl] of Object.entries(pageHandler)) {
      ;(pageHandler as any)[method] = (...args: unknown[]) => {
        onHandlerCall(method, args)
        return impl(...args)
      }
    }
  }

  const api = createTransferApi({
    pageHandler,
    bindPage: (p) => {
      page = p
    },
    isIOS: options.isIOS ?? false,
    isMobile: options.isMobile ?? false,
  })

  return api
}
