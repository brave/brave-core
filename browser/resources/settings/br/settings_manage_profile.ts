// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerPrototypeModification,
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'
import { loadTimeData } from 'chrome://resources/js/load_time_data.js'

import {
  BraveManageProfileBrowserProxy,
  CustomAvatarState
} from './brave_manage_profile_browser_proxy.js'

const kCustomAvatarRowId = 'braveCustomAvatarRow'
const kCustomAvatarPreviewId = 'braveCustomAvatarPreview'
const kCustomAvatarUploadBtnId = 'braveCustomAvatarUploadBtn'
const kCustomAvatarRemoveBtnId = 'braveCustomAvatarRemoveBtn'
const kCustomAvatarFileInputId = 'braveCustomAvatarFileInput'

// Injected as a normal `<style>` via `textContent` inside the component
// template. Do **not** use `RegisterStyleOverride` + `<style include="…">` here:
// that path registers a `dom-module` that Polymer's style gather can mis-parse,
// yielding "Could not find style data in module named null" and breaking
// subtree initialization (so `connectedCallback` / upload wiring never runs).
const kBraveCustomAvatarStyleId = 'brave-custom-avatar-style'
const kBraveCustomAvatarStyleCss = `
  .cr-row.manage-profile-section {
    padding-top: var(--leo-spacing-xl) !important;
  }

  #braveCustomAvatarRow {
    align-items: center;
    column-gap: 16px;
    display: grid;
    grid-template-columns: 72px 1fr;
    padding-inline-start: var(--cr-section-indent-width);
    padding-top: 4px;
  }

  #braveCustomAvatarPreview {
    align-items: center;
    background: var(--leo-color-container-background);
    background-position: center;
    background-repeat: no-repeat;
    background-size: cover;
    border: 1px solid var(--leo-color-divider-subtle);
    border-radius: 100%;
    box-sizing: border-box;
    display: flex;
    height: 72px;
    justify-content: center;
    position: relative;
    width: 72px;
  }

  #braveCustomAvatarPreview.is-selected {
    border-color: var(--leo-color-primary-50, #4c54d2);
  }

  #braveCustomAvatarPreview .placeholder {
    color: var(--leo-color-text-tertiary, #6b7280);
    font-size: 28px;
    font-weight: 600;
    user-select: none;
  }

  /* Hide the upload hint once a custom image is shown (no .is-empty). */
  #braveCustomAvatarPreview:not(.is-empty) .placeholder {
    display: none;
  }

  #braveCustomAvatarPreview .upload-spinner {
    animation: brave-custom-avatar-spin 0.75s linear infinite;
    border: 3px solid var(--leo-color-divider-subtle, rgba(0, 0, 0, 0.12));
    border-radius: 50%;
    border-top-color: var(--leo-color-primary-50, #4c54d2);
    box-sizing: border-box;
    display: none;
    flex-shrink: 0;
    height: 28px;
    width: 28px;
  }

  @keyframes brave-custom-avatar-spin {
    to {
      transform: rotate(360deg);
    }
  }

  #braveCustomAvatarPreview.is-uploading .placeholder {
    display: none;
  }

  #braveCustomAvatarPreview.is-uploading .upload-spinner {
    display: block;
  }

  #braveCustomAvatarPreview .check-badge {
    align-items: center;
    background: var(--leo-color-primary-50, #4c54d2);
    border: 2px solid var(--leo-color-page-background, #fff);
    border-radius: 50%;
    color: #fff;
    display: none;
    font-size: 12px;
    height: 18px;
    justify-content: center;
    line-height: 1;
    position: absolute;
    right: -4px;
    top: -4px;
    width: 18px;
  }

  #braveCustomAvatarPreview.is-selected .check-badge {
    display: flex;
  }

  #braveCustomAvatarPreview.can-activate {
    cursor: pointer;
  }

  #braveCustomAvatarPreview.is-empty:not(.is-uploading) {
    cursor: pointer;
  }

  .brave-custom-avatar-body {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .brave-custom-avatar-actions {
    display: flex;
    gap: 8px;
  }

  /* Native <button>: match leo-button filled / outline tokens (Nala). */
  #braveCustomAvatarUploadBtn,
  #braveCustomAvatarRemoveBtn {
    border-radius: var(--leo-radius-full);
    cursor: pointer;
    font: var(--leo-font-components-button-small, inherit);
    min-height: 32px;
    padding: 0 var(--leo-spacing-l);
  }

  #braveCustomAvatarUploadBtn {
    background: var(--leo-color-button-background);
    border: none;
    color: var(--leo-color-schemes-on-primary, #fff);
  }

  #braveCustomAvatarRemoveBtn {
    background: transparent;
    border: 1px solid var(--leo-color-divider-interactive);
    color: var(--leo-color-text-interactive);
  }

  #braveCustomAvatarFileInput {
    height: 1px;
    left: -9999px;
    opacity: 0;
    pointer-events: none;
    position: absolute;
    width: 1px;
  }
`

type ManageProfileHost = HTMLElement & {
  braveCustomAvatarState_: CustomAvatarState
}

const kBraveCustomAvatarWired = Symbol('braveCustomAvatarWired')

function syncBravePresetAvatarBinding_(host: ManageProfileHost) {
  if (!host.braveCustomAvatarState_.isActive) {
    return
  }
  const withAvatarProp = host as unknown as { profileAvatar_: unknown }
  withAvatarProp.profileAvatar_ = null
}

// Empty state used before the first update arrives so DOM updates never
// observe `undefined` fields.
const kEmptyAvatarState: CustomAvatarState = {
  hasSavedAvatar: false,
  isActive: false,
  dataUrl: ''
}

function braveOnCustomAvatarChanged_(
  host: ManageProfileHost, state: CustomAvatarState) {
  host.braveCustomAvatarState_ = state

  const root = host.shadowRoot
  if (!root) {
    return
  }
  const preview = root.getElementById(kCustomAvatarPreviewId)
  const removeBtn = root.getElementById(kCustomAvatarRemoveBtnId)
  if (!preview || !removeBtn) {
    return
  }

  const saved = state.hasSavedAvatar
  const active = state.isActive

  if (saved && state.dataUrl) {
    preview.style.backgroundImage = `url("${state.dataUrl}")`
    preview.classList.remove('is-empty')
    removeBtn.hidden = false
  } else if (saved) {
    preview.style.backgroundImage = ''
    preview.classList.add('is-empty')
    removeBtn.hidden = false
  } else {
    preview.style.backgroundImage = ''
    preview.classList.add('is-empty')
    removeBtn.hidden = true
  }

  if (active) {
    preview.classList.add('is-selected')
  } else {
    preview.classList.remove('is-selected')
  }

  if (saved && !active) {
    preview.classList.add('can-activate')
  } else {
    preview.classList.remove('can-activate')
  }

  syncBravePresetAvatarBinding_(host)
}

// `RegisterPolymerComponentBehaviors` is unreliable with lazy-loaded
// `manage_profile.js`. `connectedCallback` can run before the shadow tree has
// our injected nodes. `SettingsViewMixin` also uses `ready()`, which runs after
// initial layout — hook both and retry with rAF until the controls exist.
function wireBraveManageProfileCustomAvatar(host: HTMLElement, attempt = 0) {
  try {
    if ((host as unknown as Record<symbol, boolean>)[kBraveCustomAvatarWired]) {
      return
    }
    const root = host.shadowRoot
    if (!root) {
      if (attempt < 24) {
        requestAnimationFrame(() => wireBraveManageProfileCustomAvatar(
          host, attempt + 1))
      }
      return
    }

    const uploadBtn = root.getElementById(kCustomAvatarUploadBtnId)
    const removeBtn = root.getElementById(kCustomAvatarRemoveBtnId)
    const fileInput =
      root.getElementById(kCustomAvatarFileInputId) as HTMLInputElement | null
    const preview = root.getElementById(kCustomAvatarPreviewId)

    if (!uploadBtn || !removeBtn || !fileInput || !preview) {
      if (attempt < 24) {
        requestAnimationFrame(() => wireBraveManageProfileCustomAvatar(
          host, attempt + 1))
      }
      return
    }

    const typedHost = host as ManageProfileHost
    typedHost.braveCustomAvatarState_ =
      typedHost.braveCustomAvatarState_ ?? kEmptyAvatarState

    const proxy = BraveManageProfileBrowserProxy.getInstance()

    const openCustomAvatarFilePicker = () => {
      fileInput.value = ''
      fileInput.click()
    }

    uploadBtn.addEventListener('click', openCustomAvatarFilePicker)

    fileInput.addEventListener('change', async () => {
      const file = fileInput.files && fileInput.files[0]
      if (!file) {
        return
      }
      preview.classList.add('is-uploading')
      preview.setAttribute('aria-busy', 'true')
      try {
        const base64 = await fileToBase64(file)
        const result = await proxy.setCustomAvatar(base64)
        if (result.error !== undefined) {
          console.warn('[Brave Settings Overrides] setCustomAvatar failed:',
            result.error)
        }
        braveOnCustomAvatarChanged_(typedHost, result.state)
      } catch (err) {
        console.warn('[Brave Settings Overrides] Failed to set custom ' +
          'avatar:', err)
      } finally {
        preview.classList.remove('is-uploading')
        preview.removeAttribute('aria-busy')
      }
    })

    removeBtn.addEventListener('click', () => {
      try {
        proxy.removeCustomAvatar()
      } catch (err) {
        console.warn('[Brave Settings Overrides] Failed to remove custom ' +
          'avatar:', err)
      }
      // Optimistically update local state; the browser confirms via the
      // `OnCustomAvatarChanged` event when the file delete completes.
      braveOnCustomAvatarChanged_(typedHost, kEmptyAvatarState)
    })

    preview.addEventListener('click', () => {
      const st = typedHost.braveCustomAvatarState_
      if (!st.hasSavedAvatar) {
        openCustomAvatarFilePicker()
        return
      }
      if (st.isActive) {
        return
      }
      try {
        proxy.activateCustomAvatar()
      } catch (err) {
        console.warn('[Brave Settings Overrides] Failed to activate custom ' +
          'avatar:', err)
      }
    })

    proxy.callbackRouter.onCustomAvatarChanged.addListener(
      (state: CustomAvatarState) =>
        braveOnCustomAvatarChanged_(typedHost, state))

    proxy.getCustomAvatar()
      .then((state) => braveOnCustomAvatarChanged_(typedHost, state))
      .catch(() => { /* ignore - row stays in empty state */ })

    ;(host as unknown as Record<symbol, boolean>)[kBraveCustomAvatarWired] =
      true
  } catch (err) {
    console.warn('[Brave Settings Overrides] Custom-avatar wiring failed:', err)
  }
}

function scheduleWireBraveManageProfileCustomAvatar(host: HTMLElement) {
  queueMicrotask(() => wireBraveManageProfileCustomAvatar(host, 0))
}

RegisterPolymerPrototypeModification({
  'settings-manage-profile': (prototype: {
    connectedCallback?: () => void
    ready?: () => void
  }) => {
    const originalConnected = prototype.connectedCallback
    prototype.connectedCallback = function (this: HTMLElement) {
      if (originalConnected) {
        originalConnected.call(this)
      }
      scheduleWireBraveManageProfileCustomAvatar(this)
    }
    const originalReady = prototype.ready
    prototype.ready = function (this: HTMLElement) {
      if (originalReady) {
        originalReady.call(this)
      }
      scheduleWireBraveManageProfileCustomAvatar(this)
    }
  },
})

async function fileToBase64(file: File): Promise<string> {
  const buffer = await file.arrayBuffer()
  const bytes = new Uint8Array(buffer)
  // Chunk the conversion to avoid blowing the call-stack on large files.
  const CHUNK_SIZE = 0x8000
  let binary = ''
  for (let i = 0; i < bytes.length; i += CHUNK_SIZE) {
    const chunk = bytes.subarray(i, Math.min(i + CHUNK_SIZE, bytes.length))
    binary += String.fromCharCode.apply(null, Array.from(chunk))
  }
  return btoa(binary)
}

// Look up a localized string, returning `fallback` if the key isn't present
// in `loadTimeData`. `loadTimeData.getString` is strict and throws an
// assertion error when the key is missing, which would otherwise abort the
// template modification and leave the whole manage-profile page blank.
function getLocalizedString(key: string, fallback: string): string {
  try {
    if (loadTimeData.valueExists(key)) {
      return loadTimeData.getString(key)
    }
  } catch (_err) {
    // Fall through to the fallback below.
  }
  return fallback
}

// Build the upload-row markup programmatically. `innerHTML` is blocked on
// `brave://settings` by the Trusted Types CSP, so we have to construct every
// node via `document.createElement`. Using `textContent` for the labels also
// removes any escaping concerns.
function buildCustomAvatarRow(
  uploadLabel: string,
  uploadButtonLabel: string,
  removeButtonLabel: string): HTMLElement {
  const section = document.createElement('div')
  section.className = 'cr-row manage-profile-section'

  const title = document.createElement('h1')
  title.className = 'cr-title-text'
  title.textContent = uploadLabel
  section.appendChild(title)

  const rowDiv = document.createElement('div')
  rowDiv.id = kCustomAvatarRowId
  section.appendChild(rowDiv)

  const preview = document.createElement('div')
  preview.id = kCustomAvatarPreviewId
  preview.className = 'is-empty'

  const placeholder = document.createElement('span')
  placeholder.className = 'placeholder'
  placeholder.textContent = '+'
  preview.appendChild(placeholder)

  const uploadSpinner = document.createElement('div')
  uploadSpinner.className = 'upload-spinner'
  uploadSpinner.setAttribute('aria-hidden', 'true')
  preview.appendChild(uploadSpinner)

  const checkBadge = document.createElement('span')
  checkBadge.className = 'check-badge'
  checkBadge.textContent = '\u2713'
  preview.appendChild(checkBadge)
  rowDiv.appendChild(preview)

  const body = document.createElement('div')
  body.className = 'brave-custom-avatar-body'

  const actions = document.createElement('div')
  actions.className = 'brave-custom-avatar-actions'

  const uploadBtn = document.createElement('button')
  uploadBtn.type = 'button'
  uploadBtn.id = kCustomAvatarUploadBtnId
  uploadBtn.className = 'action-button'
  uploadBtn.textContent = uploadButtonLabel
  actions.appendChild(uploadBtn)

  const removeBtn = document.createElement('button')
  removeBtn.type = 'button'
  removeBtn.id = kCustomAvatarRemoveBtnId
  removeBtn.hidden = true
  removeBtn.className = 'action-button'
  removeBtn.textContent = removeButtonLabel
  actions.appendChild(removeBtn)

  body.appendChild(actions)
  rowDiv.appendChild(body)

  const fileInput = document.createElement('input')
  fileInput.id = kCustomAvatarFileInputId
  fileInput.type = 'file'
  fileInput.accept = 'image/png,image/jpeg,image/webp,image/gif'
  rowDiv.appendChild(fileInput)

  return section
}

RegisterPolymerTemplateModifications({
  'settings-manage-profile': (templateContent) => {
    // Defensive: any throw inside this callback would prevent the
    // `settings-manage-profile` element from being constructed, blanking the
    // entire manage-profile page. The custom avatar row is an additive feature
    // — if anything goes wrong, the upstream controls (name, theme, avatar
    // grid, …) must still render.
    try {
      if (!templateContent.querySelector(`#${kBraveCustomAvatarStyleId}`)) {
        const styleEl = document.createElement('style')
        styleEl.id = kBraveCustomAvatarStyleId
        styleEl.textContent = kBraveCustomAvatarStyleCss
        templateContent.insertBefore(styleEl, templateContent.firstChild)
      }

      // Locate the "Pick an avatar" section so we can inject the upload row
      // as its own section directly above it.
      const avatarSelector = templateContent.querySelector(
        '#profileAvatarSelector')
      const avatarRow = avatarSelector && avatarSelector.closest(
        '.cr-row.manage-profile-section')
      if (!avatarRow || !avatarRow.parentNode) {
        console.error('[Brave Settings Overrides] Could not find the avatar ' +
          'row in settings-manage-profile.')
        return
      }

      if (templateContent.querySelector(`#${kCustomAvatarRowId}`)) {
        return
      }

      const uploadLabel =
        getLocalizedString('braveCustomAvatarTitle', 'Upload your own image')
      const uploadButtonLabel =
        getLocalizedString('braveCustomAvatarUpload', 'Upload image')
      const removeButtonLabel =
        getLocalizedString('braveCustomAvatarRemove', 'Remove')

      const newRow = buildCustomAvatarRow(
        uploadLabel, uploadButtonLabel, removeButtonLabel)
      avatarRow.parentNode.insertBefore(newRow, avatarRow)
    } catch (err) {
      console.warn('[Brave Settings Overrides] Custom-avatar template ' +
        'modification failed:', err)
    }
  }
})

