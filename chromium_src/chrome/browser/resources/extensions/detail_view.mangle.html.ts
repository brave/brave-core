// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle, load, write, findTemplate } from 'lit_mangler'

const upstream = load('../chrome/browser/resources/extensions/detail_view.html.ts')

const incognitoTemplate = findTemplate(upstream, (template) => template.text.includes('id="allow-incognito"'))
if (!incognitoTemplate) {
    throw new Error('[Brave Extensions Overrides] Could not find incognitoSetting. Has the ID changed?')
}

mangle(incognitoTemplate, (element) => {
    const section = element.querySelector('.section-content')
    if (!section) {
        throw new Error('[Brave Extensions Overrides] Could not find .section-content. Has the ID changed?')
    }
    section.textContent = '$i18n{privateInfoWarning}'
    section.append('<span ?hidden=${(this.data as any).incognitoAccess.isActive}>$i18n{spanningInfoWarning}</span>')
    section.append('<span>$i18n{privateAndTorInfoWarning}</span>')
})

write('./detail_view.html.ts', upstream)
