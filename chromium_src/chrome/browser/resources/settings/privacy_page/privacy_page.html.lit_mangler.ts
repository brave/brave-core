// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

const getById = (root: DocumentFragment, id: string) => {
  const element = root.getElementById(id)
  if (!element) {
    throw new Error(`[Settings] Privacy page: couldn't find #${id}`)
  }
  return element
}

mangle((root) => {
  getById(root, 'siteSettingsLinkRow').insertAdjacentHTML(
    'afterend',
    '<settings-brave-personalization-options>'
    + '</settings-brave-personalization-options>')

  // Brave has its own cookies and privacy guide entry points.
  getById(root, 'thirdPartyCookiesLinkRow').setAttribute('hidden', '')
}, (t) => t.text.includes('id="siteSettingsLinkRow"'))

// The privacy guide row is in a nested template, behind
// `isPrivacyGuideAvailable`.
mangle(
  (root) => getById(root, 'privacyGuideLinkRow').setAttribute('hidden', ''),
  (t) => t.text.includes('id="privacyGuideLinkRow"'),
)
