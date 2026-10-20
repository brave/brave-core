/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { PermissionStatus } from './transfer_api'
import {
  createMockInbox,
  createMockOffers,
  createMockTransfers,
  MockOptions,
} from './transfer_api_mock'

// Named page states used by Storybook and tests.
export const mockScenarios = {
  empty: (): MockOptions => ({ devices: [] }),
  devices: (): MockOptions => ({ transfers: createMockTransfers() }),
  incomingOffer: (): MockOptions => ({ offers: createMockOffers() }),
  inbox: (): MockOptions => ({
    inbox: createMockInbox(),
    inboxTotal: 140,
  }),
  emptyInbox: (): MockOptions => ({}),
  settings: (): MockOptions => ({}),
  syncOff: (): MockOptions => ({ serviceState: { syncEnabled: false } }),
  iosPermissions: (): MockOptions => ({
    isIOS: true,
    isMobile: true,
    permissions: {
      localNetwork: PermissionStatus.kNotDetermined,
      photos: PermissionStatus.kDenied,
    },
  }),
}

export type MockScenario = keyof typeof mockScenarios
