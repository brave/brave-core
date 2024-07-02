/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Command, ClickInfo } from 'chrome://resources/js/browser_command.mojom-webui.js';

const commandNames = new Map<string, Command>(Object.entries({
  'open-rewards-onboarding': Command.kOpenRewardsOnboarding,
  'open-wallet-onboarding': Command.kOpenWalletOnboarding,
  'open-web3-settings': Command.kOpenWeb3Settings,
  'open-content-filter-settings': Command.kOpenContentFilterSettings,
  'open-shields-settings': Command.kOpenShieldsSettings,
  // TODO(zenparsing): Pending spec for shields demo.
  // 'open-shields-panel': Command.kOpenShieldsPanel,
  'open-privacy-settings': Command.kOpenPrivacySettings,
  'open-vpn-onboarding': Command.kOpenVPNOnboarding
}))

interface CommandMessage {
  command: Command
  clickInfo: ClickInfo
}

function parseClickInfo (clickInfo: any): ClickInfo {
  if (!clickInfo || typeof clickInfo !== 'object') {
    clickInfo = {}
  }

  return {
    middleButton: Boolean(clickInfo.middleButton),
    altKey: Boolean(clickInfo.altKey),
    ctrlKey: Boolean(clickInfo.ctrlKey),
    metaKey: Boolean(clickInfo.metaKey),
    shiftKey: Boolean(clickInfo.shiftKey)
  }
}

export function readCommandMessage (data: any): CommandMessage | null {
  if (!data || typeof data !== 'object') {
    data = {}
  }

  if (data.messageType !== 'browser-command') {
    return null
  }

  const command = commandNames.get(String(data.command || ''))
  if (command === undefined) {
    return null
  }

  return { command, clickInfo: parseClickInfo(data) }
}
