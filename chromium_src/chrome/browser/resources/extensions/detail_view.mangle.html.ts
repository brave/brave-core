// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle((element) => {
    const section = element.querySelector('.section-content')
    if (!section) {
        throw new Error('[Brave Extensions Overrides] Could not find .section-content. Has the ID changed?')
    }
    section.textContent = '$i18n{privateInfoWarning}'
    section.append('<span ?hidden=${!this.data.incognitoAccess.isActive}> $i18n{spanningInfoWarning}</span>')
    section.append('<span> $i18n{privateAndTorInfoWarning}</span>')
}, t => t.text.includes('id="allow-incognito"'))
