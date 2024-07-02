/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BrowserCommandProxy } from 'chrome://resources/js/browser_command/browser_command_proxy.js';
import { BraveEducationProxy } from './brave_education_proxy'
import { readCommandMessage } from './message_data_reader'

function initializeContent (serverURL: string) {
  // TODO(zenparsing): Show some kind of default or redirect if a server
  // URL cannot be found.
  if (!serverURL) {
    return
  }

  const frame = document.getElementById('content') as HTMLIFrameElement
  frame.src = serverURL

  window.addEventListener('message', (event) => {
    if (event.origin !== new URL(serverURL).origin) {
      console.warn('Invalid sender origin')
      return
    }

    const commandMessage = readCommandMessage(event.data)
    if (!commandMessage) {
      console.warn('Invalid message data')
      return
    }

    const { handler } = BrowserCommandProxy.getInstance()
    handler.canExecuteCommand(commandMessage.command).then(({ canExecute }) => {
      if (!canExecute) {
        console.warn('Received invalid command: ' + commandMessage.command)
        return
      }
      handler.executeCommand(commandMessage.command, commandMessage.clickInfo);
    });
  })
}

function onReady () {
  BraveEducationProxy
      .getInstance()
      .initialize(location.href)
      .then(initializeContent)
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', onReady)
} else {
  onReady()
}
