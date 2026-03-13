// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Mock implementations for the PSST consent dialog, for Storybook and tests.
 */

import { makeCloseable } from '$web-common/api'
import * as Mojom from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'

export function createMockConsentHelper(
  overrides: Partial<Mojom.PsstConsentHelperInterface>,
) {
  return makeCloseable({
    closeDialog: () => {},
    applyChanges: () =>{},
    ...overrides,
  })
}

export function createMockCallbackRouter(
  overrides: Partial<Mojom.PsstConsentDialogCallbackRouter>,
) {
  return makeCloseable({
    setSettingsCardData: () => {},
    onSetRequestDone: () => {},
    onSetCompleted: () => {},
    ...overrides,
  })
}

export const SAMPLE_SETTING_CARD_DATA: Mojom.SettingCardData = {
  siteName: 'example.com',
  items: [
    { description: 'Allow analytics', url: 'https://example.com/analytics' },
    { description: 'Allow marketing', url: 'https://example.com/marketing' },
    { description: 'Essential only', url: 'https://example.com/essential' },
  ],
}
