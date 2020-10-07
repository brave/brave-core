// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'
import {getSectionElement} from './basic_page.js'

RegisterPolymerTemplateModifications({
  'settings-about-page': (templateContent) => {
    const section = getSectionElement(templateContent, 'about')
    if (!section.querySelector('a#release-notes')) {
      const version = section.querySelector('#updateStatusMessage ~ .secondary')
      if (!version) {
        console.error('[Brave Settings Overrides] Could not find version div')
      }
      const parent = version.parentNode
      const wrapper = document.createElement('a')
      wrapper.setAttribute('id', 'release-notes')
      wrapper.setAttribute('target', '_blank')
      wrapper.setAttribute('rel', 'noopener noreferrer')
      wrapper.setAttribute('href', 'https://brave.com/latest/')
      parent.replaceChild(wrapper, version)
      wrapper.appendChild(version)
    }

    // Help link shown if update fails
    const updateStatusMessageLink = section.querySelector('#updateStatusMessage a')
    if (updateStatusMessageLink) {
      // <if expr="is_win">
      updateStatusMessageLink.href = 'https://support.brave.com/hc/en-us/articles/360042816611-Why-isn-t-Brave-updating-automatically-on-Windows-'
      // </if>

      // <if expr="not is_win">
      updateStatusMessageLink.href = 'https://community.brave.com?p=update_error'
      // </if>
    }
  }
})
