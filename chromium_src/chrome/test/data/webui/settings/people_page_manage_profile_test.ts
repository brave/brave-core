// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import {
  loadTimeData,
  Router,
  routes,
  StatusAction,
} from 'chrome://settings/settings.js'
// </if>

import './people_page_manage_profile_test-chromium.js'

// <if expr="is_win or is_macosx or is_linux">

import {webUIListenerCallback} from 'chrome://resources/js/cr.js'
import {PromiseResolver} from 'chrome://resources/js/promise_resolver.js'
import type {BrCustomProfileImageRowElement} from 'chrome://resources/brave/custom_profile_image_row.js'
import type {
  ManageProfileBrowserProxy,
  SettingsManageProfileElement,
} from 'chrome://settings/lazy_load.js'
import {
  ManageProfileBrowserProxyImpl,
  ProfileShortcutStatus,
} from 'chrome://settings/lazy_load.js'
import {
  assertDeepEquals,
  assertEquals,
  assertFalse,
  assertTrue,
} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'
import {TestBrowserProxy} from 'chrome://webui-test/test_browser_proxy.js'

class BraveTestManageProfileBrowserProxy extends TestBrowserProxy implements
    ManageProfileBrowserProxy {
  constructor() {
    super([
      'getAvailableIcons',
      'setProfileIconToGaiaAvatar',
      'setProfileIconToDefaultAvatar',
      'setProfileName',
      'getProfileShortcutStatus',
      'addProfileShortcut',
      'removeProfileShortcut',
    ])
  }

  getAvailableIcons() {
    this.methodCalled('getAvailableIcons')
    return Promise.resolve([
      {
        url: 'fake-icon-1.png',
        label: 'fake-icon-1',
        index: 1,
        isGaiaAvatar: false,
        selected: true,
      },
      {
        url: 'fake-icon-2.png',
        label: 'fake-icon-2',
        index: 2,
        isGaiaAvatar: false,
        selected: false,
      },
    ])
  }

  setProfileIconToGaiaAvatar() {
    this.methodCalled('setProfileIconToGaiaAvatar')
  }

  setProfileIconToDefaultAvatar(index: number) {
    this.methodCalled('setProfileIconToDefaultAvatar', [index])
  }

  setProfileName(name: string) {
    this.methodCalled('setProfileName', [name])
  }

  getProfileShortcutStatus() {
    this.methodCalled('getProfileShortcutStatus')
    return Promise.resolve(ProfileShortcutStatus.PROFILE_SHORTCUT_SETTING_HIDDEN)
  }

  addProfileShortcut() {
    this.methodCalled('addProfileShortcut')
  }

  removeProfileShortcut() {
    this.methodCalled('removeProfileShortcut')
  }
}

function createManageProfileElement(): SettingsManageProfileElement {
  const element = document.createElement('settings-manage-profile')
  document.body.append(element)
  webUIListenerCallback('sync-status-changed', {
    supervisedUser: false,
    statusAction: StatusAction.NO_ACTION,
  })
  webUIListenerCallback(
    'profile-info-changed',
    {name: 'Initial Fake Name', iconUrl: ''},
  )
  Router.getInstance().navigateTo(routes.MANAGE_PROFILE)
  return element
}

function getCustomProfileImageRow(
  manageProfile: SettingsManageProfileElement,
): BrCustomProfileImageRowElement | null {
  return manageProfile.shadowRoot!.querySelector('br-custom-profile-image-row')
}

function getRequiredElement<T extends Element>(
  root: ParentNode,
  selector: string,
): T {
  const element = root.querySelector<T>(selector)
  assertTrue(!!element, `Missing ${selector}`)
  return element
}

function getRequiredCustomProfileImageRow(
  manageProfile: SettingsManageProfileElement,
): BrCustomProfileImageRowElement {
  return getRequiredElement<BrCustomProfileImageRowElement>(
    manageProfile.shadowRoot!,
    'br-custom-profile-image-row',
  )
}

function getPreviewUrl(row: BrCustomProfileImageRowElement): string | null {
  return row.shadowRoot
    .querySelector<HTMLImageElement>('#previewImage')
    ?.getAttribute('src') ?? null
}

function useDeferredImageDecodes(): PromiseResolver<void>[] {
  const resolvers: PromiseResolver<void>[] = []
  HTMLImageElement.prototype.decode = () => {
    const resolver = new PromiseResolver<void>()
    resolvers.push(resolver)
    return resolver.promise
  }
  return resolvers
}

function selectFile(
  row: BrCustomProfileImageRowElement,
  name: string,
  type = 'image/png',
) {
  const input = getRequiredElement<HTMLInputElement>(
    row.shadowRoot,
    '#fileInput',
  )
  Object.defineProperty(input, 'files', {
    configurable: true,
    value: [new File(['image'], name, {type})],
  })
  input.dispatchEvent(new Event('change'))
}

async function settleRow(row: BrCustomProfileImageRowElement) {
  await microtasksFinished()
  await row.updateComplete
  await microtasksFinished()
  await row.updateComplete
}

function braveManageProfileFeatureDisabledTests() {
  let manageProfile: SettingsManageProfileElement

  setup(function() {
    document.body.replaceChildren()
    loadTimeData.overrideValues({profileShortcutsEnabled: false})
    ManageProfileBrowserProxyImpl.setInstance(
      new BraveTestManageProfileBrowserProxy(),
    )
    manageProfile = createManageProfileElement()
  })

  teardown(function() {
    manageProfile.remove()
  })

  test('DoesNotRenderCustomProfileImageRow', function() {
    assertFalse(loadTimeData.getBoolean('customProfileImageEnabled'))
    assertEquals(null, getCustomProfileImageRow(manageProfile))
  })
}

function braveManageProfileFeatureEnabledTests() {
  let browserProxy: BraveTestManageProfileBrowserProxy
  let createdUrls: string[]
  let manageProfile: SettingsManageProfileElement
  let originalCreateObjectUrl: typeof URL.createObjectURL
  let originalDecode: typeof HTMLImageElement.prototype.decode
  let originalRevokeObjectUrl: typeof URL.revokeObjectURL
  let revokedUrls: string[]

  setup(function() {
    document.body.replaceChildren()
    loadTimeData.overrideValues({profileShortcutsEnabled: false})

    browserProxy = new BraveTestManageProfileBrowserProxy()
    ManageProfileBrowserProxyImpl.setInstance(browserProxy)

    createdUrls = []
    revokedUrls = []
    originalCreateObjectUrl = URL.createObjectURL
    originalRevokeObjectUrl = URL.revokeObjectURL
    originalDecode = HTMLImageElement.prototype.decode
    URL.createObjectURL = () => {
      const url = `blob:custom-profile-image-${createdUrls.length + 1}`
      createdUrls.push(url)
      return url
    }
    URL.revokeObjectURL = (url: string) => revokedUrls.push(url)
    HTMLImageElement.prototype.decode = () => Promise.resolve()

    manageProfile = createManageProfileElement()
  })

  teardown(function() {
    manageProfile.remove()
    URL.createObjectURL = originalCreateObjectUrl
    URL.revokeObjectURL = originalRevokeObjectUrl
    HTMLImageElement.prototype.decode = originalDecode
  })

  test('RendersLocalizedRowBetweenPickers', async function() {
    assertTrue(loadTimeData.getBoolean('customProfileImageEnabled'))

    const row = getCustomProfileImageRow(manageProfile)
    assertTrue(!!row)
    await row.updateComplete

    const themePicker = getRequiredElement(
      manageProfile.shadowRoot!,
      'cr-theme-color-picker',
    )
    const avatarSelector = getRequiredElement(
      manageProfile.shadowRoot!,
      'cr-profile-avatar-selector',
    )
    const sections = Array.from(
      manageProfile.shadowRoot!.querySelectorAll('.manage-profile-section'),
    )
    const themeSection = themePicker.closest('.manage-profile-section')
    const customSection = row.closest('.manage-profile-section')
    const avatarSection = avatarSelector.closest('.manage-profile-section')

    assertEquals(sections.indexOf(themeSection!), sections.indexOf(customSection!) - 1)
    assertEquals(sections.indexOf(customSection!), sections.indexOf(avatarSection!) - 1)
    assertEquals('7', themePicker.getAttribute('columns'))
    assertEquals('7', avatarSelector.getAttribute('columns'))

    assertEquals(
      loadTimeData.getString('CUSTOM_PROFILE_IMAGE_UPLOAD_ACTION'),
      row.titleLabel,
    )
    assertEquals(
      loadTimeData.getString('CUSTOM_PROFILE_IMAGE_REPLACE_ACTION'),
      row.replaceLabel,
    )
    assertEquals(
      loadTimeData.getString('CUSTOM_PROFILE_IMAGE_REPLACE_TOOLTIP'),
      row.replaceTooltip,
    )
    assertEquals(
      loadTimeData.getString('CUSTOM_PROFILE_IMAGE_REMOVE_ACTION'),
      row.removeLabel,
    )
    assertEquals(
      loadTimeData.getString('CUSTOM_PROFILE_IMAGE_SELECTED_PREVIEW_LABEL'),
      row.selectedPreviewLabel,
    )
    assertEquals(
      loadTimeData.getString('CUSTOM_PROFILE_IMAGE_UPLOAD_TOOLTIP'),
      row.uploadTooltip,
    )
    assertEquals(
      loadTimeData.getString('CUSTOM_PROFILE_IMAGE_REMOVE_TOOLTIP'),
      row.removeTooltip,
    )
    assertEquals(
      loadTimeData.getString('CUSTOM_PROFILE_IMAGE_INVALID_IMAGE'),
      row.invalidImageLabel,
    )

    const title = getRequiredElement<HTMLElement>(
      customSection!,
      '.cr-title-text',
    )
    assertEquals(
      loadTimeData.getString('CUSTOM_PROFILE_IMAGE_TITLE'),
      title.textContent.trim(),
    )

    assertEquals('empty', row.state)
    assertTrue(row.hideTitle)
    const rowContainer = getRequiredElement<HTMLElement>(
      row.shadowRoot,
      '#row',
    )
    assertEquals('title', rowContainer.getAttribute('aria-labelledby'))
    const rowTitle = getRequiredElement<HTMLElement>(row.shadowRoot, '#title')
    assertEquals(row.titleLabel, rowTitle.textContent.trim())
    assertEquals('none', getComputedStyle(rowTitle).display)
    const preview = getRequiredElement<HTMLElement>(row.shadowRoot, '#preview')
    assertEquals('LEO-BUTTON', preview.tagName)
    assertEquals('plain-faint', preview.getAttribute('kind'))
    assertEquals('jumbo', preview.getAttribute('size'))
    assertTrue(preview.hasAttribute('fab'))
    assertEquals(row.uploadTooltip, preview.getAttribute('title'))
    assertEquals(
      row.uploadTooltip,
      getRequiredElement<HTMLElement>(preview, '#previewLabel').textContent,
    )
    const plusIcon = getRequiredElement<HTMLElement>(
      preview,
      '#plusIcon',
    )
    assertEquals('LEO-ICON', plusIcon.tagName)
    assertEquals('plus-add', plusIcon.getAttribute('name'))

    const uploadButton = getRequiredElement<HTMLElement>(
      row.shadowRoot,
      '#uploadButton',
    )
    assertEquals('LEO-BUTTON', uploadButton.tagName)
    assertEquals('filled', uploadButton.getAttribute('kind'))
    assertEquals('small', uploadButton.getAttribute('size'))
    assertEquals(row.titleLabel, uploadButton.textContent.trim())
    assertEquals(
      'image/*',
      getRequiredElement<HTMLInputElement>(row.shadowRoot, '#fileInput').accept,
    )
    assertEquals(null, row.shadowRoot.querySelector('#removeButton'))
  })

  test('OpensFilePickerFromPreviewAndUploadButton', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    await row.updateComplete

    const fileInput = getRequiredElement<HTMLInputElement>(
      row.shadowRoot,
      '#fileInput',
    )
    let inputClickCount = 0
    fileInput.click = () => ++inputClickCount

    getRequiredElement<HTMLElement>(row.shadowRoot, '#preview').click()
    assertEquals(1, inputClickCount)
    getRequiredElement<HTMLElement>(row.shadowRoot, '#uploadButton').click()
    assertEquals(2, inputClickCount)
  })

  test('UploadsValidImage', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    selectFile(row, 'first.png')
    await settleRow(row)

    assertEquals('saved-active', row.state)
    assertEquals(createdUrls[0], getPreviewUrl(row))
    const preview = getRequiredElement<HTMLElement>(row.shadowRoot, '#preview')
    assertEquals(row.selectedPreviewLabel, preview.getAttribute('aria-label'))
    const selectedIndicator = getRequiredElement<HTMLElement>(
      row.shadowRoot,
      '#selectedIndicator',
    )
    assertEquals('LEO-ICON', selectedIndicator.tagName)
    assertEquals('check-normal', selectedIndicator.getAttribute('name'))

    const replaceButton = getRequiredElement<HTMLElement>(
      row.shadowRoot,
      '#uploadButton',
    )
    assertEquals('LEO-BUTTON', replaceButton.tagName)
    assertEquals('filled', replaceButton.getAttribute('kind'))
    assertEquals('small', replaceButton.getAttribute('size'))
    assertEquals(row.replaceLabel, replaceButton.textContent.trim())
    assertEquals(row.replaceTooltip, replaceButton.getAttribute('title'))
    assertEquals(row.replaceTooltip, replaceButton.getAttribute('aria-label'))
    const removeButton = getRequiredElement<HTMLElement>(
      row.shadowRoot,
      '#removeButton',
    )
    assertEquals('LEO-BUTTON', removeButton.tagName)
    assertEquals('outline', removeButton.getAttribute('kind'))
    assertEquals('small', removeButton.getAttribute('size'))
    assertDeepEquals([], revokedUrls)
  })

  test('UsesConfiguredActionSpacing', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    selectFile(row, 'first.png')
    await settleRow(row)

    const actions = getRequiredElement<HTMLElement>(row.shadowRoot, '#actions')
    assertEquals('12px', getComputedStyle(actions).gap)
  })

  test('ReplacesImageAndChangesPreviewUrl', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    selectFile(row, 'first.png')
    await settleRow(row)
    const firstPreviewUrl = getPreviewUrl(row)

    selectFile(row, 'second.png')
    await settleRow(row)

    assertEquals(createdUrls[1], getPreviewUrl(row))
    assertFalse(firstPreviewUrl === getPreviewUrl(row))
    assertDeepEquals([createdUrls[0]], revokedUrls)
  })

  test('RemovesImageAndRevokesUrl', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    selectFile(row, 'first.png')
    await settleRow(row)

    getRequiredElement<HTMLElement>(row.shadowRoot, '#removeButton').click()
    await settleRow(row)

    assertEquals('empty', row.state)
    assertEquals(null, getPreviewUrl(row))
    assertEquals(null, row.shadowRoot.querySelector('#removeButton'))
    assertDeepEquals([createdUrls[0]], revokedUrls)
  })

  test('RejectsNonImageFile', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    selectFile(row, 'not-an-image.txt', 'text/plain')
    await settleRow(row)

    assertEquals('empty', row.state)
    assertEquals(null, getPreviewUrl(row))
    assertDeepEquals([], createdUrls)
    assertDeepEquals([], revokedUrls)
    assertEquals(
      row.invalidImageLabel,
      getRequiredElement(row.shadowRoot, '#fileError').textContent,
    )
  })

  test('RejectsCorruptImageAndPreservesPreview', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    selectFile(row, 'first.png')
    await settleRow(row)
    const firstPreviewUrl = getPreviewUrl(row)

    HTMLImageElement.prototype.decode = () =>
      Promise.reject(new Error('Image decode failed'))
    selectFile(row, 'corrupt.png')
    await settleRow(row)

    assertEquals('saved-active', row.state)
    assertEquals(firstPreviewUrl, getPreviewUrl(row))
    assertEquals(
      row.invalidImageLabel,
      getRequiredElement(row.shadowRoot, '#fileError').textContent,
    )
    assertDeepEquals([createdUrls[1]], revokedUrls)
  })

  test('NewestOverlappingUploadWins', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    const decodeResolvers = useDeferredImageDecodes()

    selectFile(row, 'stale.png')
    selectFile(row, 'newest.png')
    assertEquals(2, decodeResolvers.length)

    decodeResolvers[1]!.resolve()
    await settleRow(row)
    assertEquals('saved-active', row.state)
    assertEquals(createdUrls[1], getPreviewUrl(row))

    decodeResolvers[0]!.resolve()
    await settleRow(row)
    assertEquals(createdUrls[1], getPreviewUrl(row))
    assertDeepEquals([createdUrls[0]], revokedUrls)
  })

  test('RemovalWhileDecodePendingCleansUpUrls', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    selectFile(row, 'active.png')
    await settleRow(row)

    const decodeResolvers = useDeferredImageDecodes()
    selectFile(row, 'pending.png')
    assertEquals(1, decodeResolvers.length)

    getRequiredElement<HTMLElement>(row.shadowRoot, '#removeButton').click()
    await settleRow(row)
    assertEquals('empty', row.state)
    assertDeepEquals([createdUrls[0]], revokedUrls)

    decodeResolvers[0]!.resolve()
    await settleRow(row)
    assertEquals('empty', row.state)
    assertEquals(null, getPreviewUrl(row))
    assertDeepEquals(createdUrls, revokedUrls)
  })

  test('DisconnectWhileDecodePendingRevokesUrl', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    const decodeResolvers = useDeferredImageDecodes()
    selectFile(row, 'pending.png')
    assertEquals(1, decodeResolvers.length)

    manageProfile.remove()
    decodeResolvers[0]!.resolve()
    await settleRow(row)

    assertEquals('empty', row.state)
    assertEquals(null, getPreviewUrl(row))
    assertDeepEquals([createdUrls[0]], revokedUrls)
  })

  test('DisconnectWhileActiveRevokesUrl', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    selectFile(row, 'active.png')
    await settleRow(row)

    manageProfile.remove()
    await settleRow(row)

    assertEquals('empty', row.state)
    assertEquals(null, getPreviewUrl(row))
    assertDeepEquals([createdUrls[0]], revokedUrls)
  })

  test('RecreatingPageDropsSessionImageAndRevokesUrl', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    selectFile(row, 'active.png')
    await settleRow(row)
    assertEquals(createdUrls[0], getPreviewUrl(row))

    manageProfile.remove()
    assertDeepEquals([createdUrls[0]], revokedUrls)
    manageProfile = createManageProfileElement()

    const recreatedRow = getRequiredCustomProfileImageRow(manageProfile)
    await recreatedRow.updateComplete
    assertEquals('empty', recreatedRow.state)
    assertEquals(null, getPreviewUrl(recreatedRow))
    assertDeepEquals([createdUrls[0]], revokedUrls)
  })

  test('KeepsPresetAvatarSelectionWorkingAfterUpload', async function() {
    const row = getRequiredCustomProfileImageRow(manageProfile)
    selectFile(row, 'active.png')
    await settleRow(row)
    const customPreviewUrl = getPreviewUrl(row)

    await browserProxy.whenCalled('getAvailableIcons')
    await microtasksFinished()
    const avatarSelector = getRequiredElement(
      manageProfile.shadowRoot!,
      'cr-profile-avatar-selector',
    )
    const avatar = getRequiredElement<HTMLElement>(
      avatarSelector.shadowRoot!,
      '.avatar-container > .avatar',
    )
    avatar.click()

    const args = await browserProxy.whenCalled('setProfileIconToDefaultAvatar')
    assertEquals(1, args[0])
    assertEquals('saved-active', row.state)
    assertEquals(customPreviewUrl, getPreviewUrl(row))
  })
}

if (loadTimeData.getBoolean('customProfileImageEnabled')) {
  suite(
    'BraveManageProfileFeatureEnabledTests',
    braveManageProfileFeatureEnabledTests,
  )
} else {
  suite(
    'BraveManageProfileFeatureDisabledTests',
    braveManageProfileFeatureDisabledTests,
  )
}

// </if>
