// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mount } from './email_aliases'

const signInRoot = document.querySelector<HTMLElement>('#signInRoot')
const manageRoot = document.querySelector<HTMLElement>('#manageRoot')
if (!signInRoot || !manageRoot) {
  throw new Error('Unable to find Email Aliases page mount points')
}

customElements.whenDefined('brave-account-row').then(() => {
  mount(signInRoot, manageRoot, {
    onLoggedInChange: () => {},
    styleTarget: document.head,
  })
})
