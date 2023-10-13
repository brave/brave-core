// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {html, RegisterPolymerTemplateModifications, RegisterStyleOverride} from 'chrome://resources/brave/polymer_overriding.js'

import {getSectionElement} from './basic_page.js'

import {sanitizeInnerHtml} from 'chrome://resources/js/parse_html_subset.js'

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

const extractVersions = (versionElement: HTMLElement) => {
  const [ _, braveVersion, chromiumVersion, build ] = versionElement
    .innerHTML
    .match(/^Version\s([\d+\.?]+)\sChromium:\s([\d+\.?]+)\s(.*)$/)

  return { braveVersion, build, chromiumVersion }
}

const buildBraveVersionLink = (braveVersion: string, build: string) => {
  const wrapper = document.createElement('a')
  wrapper.setAttribute('id', 'release-notes')
  wrapper.setAttribute('target', '_blank')
  wrapper.setAttribute('rel', 'noopener noreferrer')
  wrapper.setAttribute('href', 'https://brave.com/latest/')
  wrapper.innerHTML = sanitizeInnerHtml(`Brave ${braveVersion} ${build}`)

  return wrapper
}

const buildChromiumVersionElement = (chromiumVersion:string, braveVersionLink: HTMLElement) => {
  const chromiumElement = document.createElement('div')
  chromiumElement.classList.add("secondary")
  chromiumElement.innerHTML = sanitizeInnerHtml(`Chromium: ${chromiumVersion}`)

  braveVersionLink.after(chromiumElement)
}

RegisterPolymerTemplateModifications({
  'settings-about-page': (templateContent) => {
    const section = getSectionElement(templateContent, 'about')
    if (!section.querySelector('a#release-notes')) {
      const version = section.querySelector('#updateStatusMessage ~ .secondary')
      if (!version) {
        console.error('[Brave Settings Overrides] Could not find version div')
      }

      const { braveVersion, build, chromiumVersion } = extractVersions(version)
      const braveVersionLink = buildBraveVersionLink(braveVersion, build)
      version.parentNode.replaceChild(braveVersionLink, version)
      buildChromiumVersionElement(chromiumVersion, braveVersionLink)
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
