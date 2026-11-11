// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Keep upstream profile-creation coverage alongside tests of the shared image row.
import './profile_customization_test-chromium.js'

// <if expr="enable_brave_custom_profile_image_webui">

import type {ProfileCustomizationAppElement} from 'chrome://profile-customization/profile_customization_app.js'
import {ProfileCustomizationBrowserProxyImpl} from 'chrome://profile-customization/profile_customization_browser_proxy.js'
import type {BrCustomProfileImageRowElement} from 'chrome://resources/brave/custom_profile_image_row.js'
import {loadTimeData} from 'chrome://resources/js/load_time_data.js'
import {
  assertEquals,
  assertFalse,
  assertTrue,
} from 'chrome://webui-test/chai_assert.js'
import {
  eventToPromise,
  isVisible,
  microtasksFinished,
} from 'chrome://webui-test/test_util.js'

import {TestProfileCustomizationBrowserProxy} from './test_profile_customization_browser_proxy.js'

const featureEnabled = loadTimeData.getBoolean('customProfileImageEnabled')

function getRequiredElement<T extends Element = HTMLElement>(
  root: ParentNode,
  selector: string,
): T {
  const element = root.querySelector<T>(selector)
  assertTrue(!!element, `Missing ${selector}`)
  return element
}

function getRow(
  app: ProfileCustomizationAppElement,
): BrCustomProfileImageRowElement {
  return getRequiredElement(app.shadowRoot, 'br-custom-profile-image-row')
}

function getNativeButton(
  row: BrCustomProfileImageRowElement,
  selector: string,
): HTMLButtonElement {
  const control = getRequiredElement(row.shadowRoot, selector)
  assertTrue(!!control.shadowRoot)
  return getRequiredElement(control.shadowRoot, 'button')
}

async function selectImage(row: BrCustomProfileImageRowElement) {
  const originalCreateObjectUrl = URL.createObjectURL
  const originalDecode = HTMLImageElement.prototype.decode
  try {
    // These tests need a selected image for layout and navigation. The native
    // chooser test covers real file delivery and decoding.
    URL.createObjectURL = () => 'blob:custom-profile-image'
    HTMLImageElement.prototype.decode = () => Promise.resolve()
    const input = getRequiredElement<HTMLInputElement>(row.shadowRoot, '#fileInput')
    Object.defineProperty(input, 'files', {
      value: [new File(['image'], 'preview.png', {type: 'image/png'})],
    })
    input.dispatchEvent(new Event('change'))
    await microtasksFinished()
    return getRequiredElement<HTMLImageElement>(row.shadowRoot, '#previewImage')
  } finally {
    URL.createObjectURL = originalCreateObjectUrl
    HTMLImageElement.prototype.decode = originalDecode
  }
}

// Shared upload, validation, and removal behavior is covered by the Settings
// consumer. These tests cover the profile-creation host and its navigation.
function profileCustomizationTests() {
  let app: ProfileCustomizationAppElement
  let browserProxy: TestProfileCustomizationBrowserProxy

  async function createApp(isLocalProfileCreation = true) {
    document.body.replaceChildren()
    loadTimeData.overrideValues({
      profileName: 'Test profile',
      isLocalProfileCreation,
    })
    browserProxy = new TestProfileCustomizationBrowserProxy()
    browserProxy.setProfileInfo({
      backgroundColor: 'rgb(0, 255, 0)',
      pictureUrl: 'chrome://theme/IDR_PROFILE_AVATAR_1',
      isManaged: false,
      hasEnterpriseLabel: false,
      welcomeTitle: '',
    })
    ProfileCustomizationBrowserProxyImpl.setInstance(browserProxy)
    app = document.createElement('profile-customization-app')
    document.body.append(app)
    await browserProxy.whenCalled('initialized')
    await microtasksFinished()
  }

  async function openAvatarPicker() {
    const avatarView = getRequiredElement(app.shadowRoot, '#selectAvatarDialog')
    const customizeView = getRequiredElement(app.shadowRoot, '#customizeDialog')
    const entered = eventToPromise('view-enter-finish', avatarView)
    const exited = eventToPromise('view-exit-finish', customizeView)
    getRequiredElement(app.shadowRoot, '#customizeAvatarIcon').click()
    await Promise.all([entered, exited])
    await microtasksFinished()
    assertTrue(isVisible(avatarView))
    assertFalse(isVisible(customizeView))
  }

  async function selectPreset() {
    const selector = getRequiredElement(
      app.shadowRoot,
      'cr-profile-avatar-selector',
    )
    getRequiredElement<HTMLElement>(
      selector.shadowRoot!,
      '.avatar[data-index="1"]',
    ).click()
    await microtasksFinished()
    const customizeView = getRequiredElement(app.shadowRoot, '#customizeDialog')
    const avatarView = getRequiredElement(app.shadowRoot, '#selectAvatarDialog')
    const entered = eventToPromise('view-enter-finish', customizeView)
    const exited = eventToPromise('view-exit-finish', avatarView)
    getRequiredElement(app.shadowRoot, '#selectAvatarConfirmButton').click()
    assertEquals(1, await browserProxy.whenCalled('setAvatarIcon'))
    await Promise.all([entered, exited])
    assertTrue(isVisible(customizeView))
  }

  setup(async function() {
    await createApp()
  })

  teardown(function() {
    app.remove()
  })

  if (!featureEnabled) {
    test('KeepsOriginalAvatarPickerWhenDisabled', async function() {
      await openAvatarPicker()
      assertEquals(null, app.shadowRoot.querySelector('br-custom-profile-image-row'))
      await selectPreset()
    })
    return
  }

  test('ShowsRowBeforePresetsInAvatarView', async function() {
    const row = getRow(app)
    assertFalse(isVisible(row))
    await openAvatarPicker()
    assertTrue(isVisible(row))
    const selector = getRequiredElement(app.shadowRoot, 'cr-profile-avatar-selector')
    assertTrue(row.getBoundingClientRect().bottom <=
      selector.getBoundingClientRect().top)
    assertFalse(isVisible(getRequiredElement(row.shadowRoot, '#title')))
  })

  const directions = ['ltr', 'rtl']
  directions.forEach(direction => {
    test(`AlignsPreviewWithPresets_${direction}`, async function() {
      app.dir = direction
      await openAvatarPicker()
      const row = getRow(app)
      const selector = getRequiredElement(
        app.shadowRoot,
        'cr-profile-avatar-selector',
      )
      const preset = getRequiredElement(
        selector.shadowRoot!,
        '.avatar[data-index="0"]',
      ).getBoundingClientRect()
      const edge = direction === 'ltr' ? 'left' : 'right'
      const emptyPreview = getNativeButton(row, '#preview').getBoundingClientRect()
      assertTrue(
        Math.abs(emptyPreview[edge] - preset[edge]) <= 1,
        `empty preview ${direction}: preview=${emptyPreview[edge]}, `
          + `preset=${preset[edge]}, difference=${emptyPreview[edge] - preset[edge]}`,
      )

      await selectImage(row)
      const selectedPreview = getRequiredElement(
        row.shadowRoot,
        '#preview',
      ).getBoundingClientRect()
      assertTrue(
        Math.abs(selectedPreview[edge] - preset[edge]) <= 1,
        `selected preview ${direction}: preview=${selectedPreview[edge]}, `
          + `preset=${preset[edge]}, difference=${selectedPreview[edge] - preset[edge]}`,
      )
    })
  })

  test('KeepsPresetSelectionWorkingAfterUpload', async function() {
    await openAvatarPicker()
    const row = getRow(app)
    const image = await selectImage(row)
    const url = image.src
    await selectPreset()
    await openAvatarPicker()
    assertEquals(
      url,
      getRequiredElement<HTMLImageElement>(row.shadowRoot, '#previewImage').src,
    )
  })

  test('DoesNotAddRowToNonLocalCustomization', async function() {
    await createApp(false)
    assertEquals(null, app.shadowRoot.querySelector('br-custom-profile-image-row'))
  })
}

suite(
  featureEnabled
    ? 'BraveProfileCustomizationFeatureEnabledTests'
    : 'BraveProfileCustomizationFeatureDisabledTests',
  profileCustomizationTests,
)

// </if>
