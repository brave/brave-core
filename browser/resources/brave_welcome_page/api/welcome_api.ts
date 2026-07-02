/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  WelcomePageHandler,
  WelcomePageHandlerInterface,
} from 'gen/brave/browser/ui/webui/brave_welcome_page/brave_welcome_page.mojom.m.js'

import { sendWithPromise } from 'chrome://resources/js/cr.js'
import { createInterfaceApi } from '$web-common/api'

// Type returned from requestDefaultBrowserState message.
export interface DefaultBrowserInfo {
  canBeDefault: boolean
  isDefault: boolean
  isDisabledByPolicy: boolean
  isUnknownError: boolean
}

interface ApiInit {
  welcomePageHandler: WelcomePageHandlerInterface
  messages: {
    getDefaultBrowserInfo: () => Promise<DefaultBrowserInfo>
    setAsDefaultBrowser: () => void
  }
}

function defaultInit(): ApiInit {
  return {
    welcomePageHandler: WelcomePageHandler.getRemote(),
    messages: {
      getDefaultBrowserInfo() {
        return sendWithPromise('requestDefaultBrowserState')
      },
      setAsDefaultBrowser() {
        chrome.send('setAsDefaultBrowser')
      },
    },
  }
}

export function createWelcomeApi(init = defaultInit()) {
  const api = createInterfaceApi({
    endpoints: {
      getDefaultBrowserState: {
        query: () => init.messages.getDefaultBrowserInfo(),
      },
    },

    actions: {
      setAsDefaultBrowser: init.messages.setAsDefaultBrowser,
    },
  })

  return api
}

export type WelcomeApi = ReturnType<typeof createWelcomeApi>
