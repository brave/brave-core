/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

testing.addTest(async () => {
  let { waitForElement, waitForShadowElement } = testing

  let startButton = await waitForShadowElement([
    '.onboarding-button',
    'button'
  ])
  startButton.click()

  let select = await waitForElement('.country-select-modal select')
  select.value = 'US'
  select.dispatchEvent(new Event('change', { bubbles: true }))

  let continueButton = await waitForShadowElement([
    '.country-select-modal .continue-button',
    'button:enabled'
  ])
  continueButton.click()

  continueButton = await waitForShadowElement([
    '.wdp-opt-in-modal .continue-button',
    'button'
  ])
  continueButton.click()

  await waitForElement('.onboarding-success')

})
