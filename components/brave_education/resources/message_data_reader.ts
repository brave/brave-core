/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Command, ClickInfo } from 'chrome://resources/js/browser_command.mojom-webui.js';

const commandNames = new Map<string, Command>(Object.entries({
  'open-rewards-onboarding': Command.kOpenRewardsOnboarding,
  'open-wallet-onboarding': Command.kOpenWalletOnboarding,
  'open-vpn-onboarding': Command.kOpenVPNOnboarding,
  'open-ai-chat': Command.kOpenAIChat
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

// Reads and validates a message posted from an education page iframe and
// returns a `CommandMessage` that can be dispatched to the browser. If the
// message is invalid, an error is thrown.
export function readCommandMessage (data: any): CommandMessage {
  if (!data || typeof data !== 'object') {
    data = {}
  }

  if (data.messageType !== 'browser-command') {
    throw new Error(`Invalid messageType "${ data.messageType }"`)
  }

  const command = commandNames.get(String(data.command || ''))
  if (command === undefined) {
    throw new Error(`Unrecognized command "${ data.command }"`)
  }

  return { command, clickInfo: parseClickInfo(data) }
}
