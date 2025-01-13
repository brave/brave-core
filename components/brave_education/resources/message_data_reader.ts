/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Command } from 'gen/brave/ui/webui/resources/js/brave_browser_command/brave_browser_command.mojom.m.js'

const commandNames = new Map<string, Command>(Object.entries({
  'open-rewards-onboarding': Command.kOpenRewardsOnboarding,
  'open-wallet-onboarding': Command.kOpenWalletOnboarding,
  'open-vpn-onboarding': Command.kOpenVPNOnboarding,
  'open-ai-chat': Command.kOpenAIChat
}))

interface CommandMessage {
  command: Command
}

// Reads and validates a message posted from an education page iframe and
// returns a `CommandMessage` that can be dispatched to the browser. If the
// message is invalid, an error is thrown.
export function readCommandMessage(data: any): CommandMessage {
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

  return { command }
}
