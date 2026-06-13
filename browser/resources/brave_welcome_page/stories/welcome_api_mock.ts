/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { WelcomeApi, createWelcomeApi, mojom } from '../api/welcome_api'

export function createWelcomeApiMock(): WelcomeApi {
  const api = createWelcomeApi({
    welcomePageHandler: {
      setWelcomePage(page) {},
      getColorScheme: async () => ({
        colorScheme: mojom.ColorScheme.kSystem,
      }),
      setColorScheme: async (colorScheme) => {},
      getVerticalTabsEnabled: async () => ({
        enabled: false,
      }),
      setVerticalTabsEnabled: async (enabled) => {},
    },
    welcomePageEventSource: {
      addListeners: (listeners) => () => {},
    },
  })

  return api
}
