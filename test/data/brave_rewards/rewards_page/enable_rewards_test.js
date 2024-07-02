/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

testing.addTest(async () => {
  let { addRequestHandler, waitForElement, waitForShadowElement } = testing

  addRequestHandler((url, method) => {
    if (url.pathname === '/v4/wallets' && method === 'POST') {
      return [201, { paymentId: '33fe956b-ed15-515b-bccd-b6cc63a80e0e' }]
    }
    return null
  })

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

  await waitForElement('.onboarding-success')

})
