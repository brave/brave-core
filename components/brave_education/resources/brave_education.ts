/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BrowserCommandProxy } from 'chrome://resources/js/browser_command/browser_command_proxy.js';
import { EducationPageProxy } from './education_page_proxy';
import { readCommandMessage } from './message_data_reader'

function pageReady() {
  return new Promise<void>((resolve) => {
    if (document.readyState === 'loading') {
      document.addEventListener('DOMContentLoaded', () => resolve())
    } else {
      resolve()
    }
  })
}

async function handleMessage(data: any) {
  // Read the message provided by the website. If the message is invalid, then
  // an error will be thrown and will appear on the console for debugging.
  const message = readCommandMessage(data)

  // Check to make sure that the command can be executed for the current
  // profile.
  const { handler } = BrowserCommandProxy.getInstance()
  const { canExecute } = await handler.canExecuteCommand(message.command)
  if (!canExecute) {
    console.warn(`Command "${ message.command }" cannot be executed`)
    return
  }

  // Execute the command.
  handler.executeCommand(message.command, message.clickInfo);
}

async function main() {
  await pageReady()

  // Get the website URL that corresponds to this webUI URL. If there is no
  // match, then do not load anything into the iframe and leave the page empty.
  const { handler } = EducationPageProxy.getInstance()
  const { serverUrl } = await handler.getServerUrl(location.href)
  if (!serverUrl) {
    console.warn('No matching server URL')
    return
  }

  // Load the education content into the iframe.
  const frame = document.getElementById('content') as HTMLIFrameElement
  frame.src = serverUrl

  // Attach a postMessage handler for messages originating from the iframe.
  window.addEventListener('message', (event) => {
    if (event.origin === 'https://brave.com' ||
        event.origin === 'chrome://webui-test') {
      handleMessage(event.data)
    }
  })
}

main()
