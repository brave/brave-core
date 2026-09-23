/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

testing.addTest(async () => {
  let { waitForElement, waitForShadowElement } = testing

  let consentCheck = await waitForShadowElement([
    '.reset-modal leo-checkbox',
    'input[type=checkbox]'
  ])
  consentCheck.checked = true
  consentCheck.dispatchEvent(new Event('change', { bubbles: true }))

  let button = await waitForShadowElement([
    '.reset-modal .reset-button',
    'button:enabled'
  ])
  button.click()

  await waitForElement('.onboarding')
})
