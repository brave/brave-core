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

  let sendButton = await waitForShadowElement([
    '.send-button',
    'button:enabled'
  ]);
  sendButton.click()

  await waitForElement('.contribution-success')
})
