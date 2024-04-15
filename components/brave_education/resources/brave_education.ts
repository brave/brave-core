/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { EducationPageProxy } from './education_page_proxy'
import { readCommandMessage } from './message_data_reader'

async function handleMessage(data: any) {
  // Read the message provided by the website. If the message is invalid, then
  // an error will be thrown and will appear on the console for debugging.
  const message = readCommandMessage(data)

  // Execute the command.
  const { handler } = EducationPageProxy.getInstance()
  const { executed } = await handler.executeCommand(message.command);
  if (!executed) {
    console.warn(`Command "${ message.command }" could not be executed`)
  }
}

async function main() {
  // Get the website URL that corresponds to this webUI URL. If there is no
  // match, then do not load anything into the iframe and leave the page empty.
  const { handler } = EducationPageProxy.getInstance()
  const { serverUrl } = await handler.getServerUrl()
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
