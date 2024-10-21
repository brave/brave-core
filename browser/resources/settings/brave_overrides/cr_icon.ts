// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CrIconElement } from '//resources/cr_elements/cr_icon/cr_icon.js'
import { iconMap, leoIcons } from './iron_icon.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import { injectStyle } from '//resources/brave/lit_overriding.js'

injectStyle(CrIconElement, css`:host {
  --leo-icon-size: var(--iron-icon-width, 24px);
  --leo-icon-color: var(--iron-icon-fill-color, currentColor);
}`)

const old = (CrIconElement.prototype as any).updateIcon_;
(CrIconElement.prototype as any).updateIcon_ = function (...args: any) {
  const removeAllOfType = (type: string) => {
    for (const node of this.shadowRoot!.querySelectorAll(type)) node.remove()
  }

  const name = iconMap[this.icon]
  if (name || leoIcons.has(this.icon)) {
    removeAllOfType('svg')

    let leoIcon = this.shadowRoot!.querySelector('leo-icon')
    if (!leoIcon) {
      leoIcon = document.createElement('leo-icon')
      this.shadowRoot!.append(leoIcon)
    }
    leoIcon.setAttribute('name', name ?? this.icon)
  } else {
    removeAllOfType('leo-icon')
    old.apply(this, args)
  }
}
