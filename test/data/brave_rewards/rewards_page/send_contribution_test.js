/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

testing.addTest(async () => {
  let { waitForElement, waitForShadowElement } = testing

  let contributeButton = await waitForShadowElement([
    '.contribute-button',
    'button'
  ])
  contributeButton.click()

  let radio = await waitForShadowElement([
    '.options .option [name=amount]',
    'input[type=radio]'
  ])
  radio.click()

  // Click in the same task that enables the button. Polling for `:enabled`
  // can land after the page has attached the matching click handler, which
  // hides a handler that lags behind the selected amount. The inner button can
  // be replaced when its label changes, so look it up again on every mutation.
  let host = await waitForElement('.send-button')
  await waitForShadowElement(['.send-button', 'button'])
  let sendButton = await new Promise((resolve) => {
    let findEnabled = () => host.shadowRoot.querySelector('button:enabled')
    let observer = new MutationObserver(() => {
      let button = findEnabled()
      if (button) {
        observer.disconnect()
        resolve(button)
      }
    })
    observer.observe(host.shadowRoot, {
      subtree: true,
      childList: true,
      attributes: true,
      attributeFilter: ['disabled']
    })
    let button = findEnabled()
    if (button) {
      observer.disconnect()
      resolve(button)
    }
  })
  sendButton.click()

  await waitForElement('.contribution-success')
})
