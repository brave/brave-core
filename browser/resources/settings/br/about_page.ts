/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {
  RegisterPolymerTemplateModifications,
  RegisterStyleOverride
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterStyleOverride(
  'settings-about-page',
  html`
    <style>
      #release-notes {
        display: block;
        margin-inline-start: unset;
      }
    </style>
  `
)

const extractVersions = (versionElement: Element) => {
  const [ , braveVersion, chromiumVersion, build ] = versionElement
    .textContent?.match(/(\d+\.\d+(?:\.\d+)*)\D+(\d+\.\d+(?:\.\d+)*)(.*)/) ?? []

  return { braveVersion, build, chromiumVersion }
}

const buildBraveVersionLink = (braveVersion: string, build: string) => {
  const wrapper = document.createElement('a')
  wrapper.setAttribute('id', 'release-notes')
  wrapper.setAttribute('target', '_blank')
  wrapper.setAttribute('rel', 'noopener noreferrer')
  wrapper.setAttribute('href', 'https://brave.com/latest/')
  wrapper.textContent = `Brave ${braveVersion} ${build}`

  return wrapper
}

const buildChromiumVersionElement = (chromiumVersion:string) => {
  const chromiumElement = document.createElement('div')
  chromiumElement.classList.add("secondary")
  chromiumElement.textContent = `Chromium: ${chromiumVersion}`

  return chromiumElement
}

RegisterPolymerTemplateModifications({
  'settings-about-page': (templateContent) => {
    if (!templateContent.querySelector('a#release-notes')) {
      const version =
        templateContent.querySelector('#updateStatusMessage ~ .secondary')
      if (!version) {
        console.error('[Settings] Could not find version div')
        return
      }

      // Remove the class from the version, so we take the link styling.
      version.removeAttribute('class')

      const wrapper = document.createElement('a')
      wrapper.setAttribute('id', 'release-notes')
      wrapper.setAttribute('target', '_blank')
      wrapper.setAttribute('rel', 'noopener noreferrer')
      wrapper.setAttribute('href', 'https://brave.com/latest/')

      const parent = version.parentNode
      parent?.replaceChild(wrapper, version)
      wrapper.appendChild(version)

      const { braveVersion, build, chromiumVersion } = extractVersions(version)
      const braveVersionLink = buildBraveVersionLink(braveVersion, build)
      version.parentNode?.replaceChild(braveVersionLink, version)

      const chromiumVersionElement = buildChromiumVersionElement(chromiumVersion)
      braveVersionLink.after(chromiumVersionElement)
    }

    // Help link shown if update fails
    const updateStatusMessageLink =
      templateContent.querySelector('#updateStatusMessage a')
    if (updateStatusMessageLink) {
      // <if expr="is_win">
      updateStatusMessageLink.href =
        'https://support.brave.app/hc/en-us/articles/360042816611-Why-isn-t-Brave-updating-automatically-on-Windows-'
      // </if>

      // <if expr="not is_win">
        updateStatusMessageLink.href =
          'https://community.brave.app?p=update_error'
      // </if>
    }
  }
})
