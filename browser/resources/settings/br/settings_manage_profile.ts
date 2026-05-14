// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerPrototypeModification,
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'
import 'chrome://resources/brave/leo.bundle.js'
import { loadTimeData } from 'chrome://resources/js/load_time_data.js'

import {
  BraveManageProfileBrowserProxy,
  CustomAvatarState
} from './brave_manage_profile_browser_proxy.js'

const kCustomAvatarRowId = 'braveCustomAvatarRow'
const kCustomAvatarPreviewId = 'braveCustomAvatarPreview'
const kCustomAvatarImageId = 'braveCustomAvatarImage'
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
    border-color: var(--leo-color-primary-50);
  }

  #braveCustomAvatarPreview .placeholder {
    --leo-icon-color: var(--leo-color-text-tertiary);
    --leo-icon-size: 28px;
    user-select: none;
  }

  /* Hide the upload hint once a custom image is shown (no .is-empty). */
  #braveCustomAvatarPreview:not(.is-empty) .placeholder {
    display: none;
  }

  /*
   * The avatar image fills the circular preview container. Rendered via a
   * real <img> (rather than a CSS background-image: url("...")) so the
   * server-built data URL never flows through a CSS string and can't escape
   * the url() context regardless of its contents.
   */
  #braveCustomAvatarImage {
    border-radius: 100%;
    display: none;
    height: 100%;
    object-fit: cover;
    width: 100%;
  }

  #braveCustomAvatarPreview:not(.is-empty) #braveCustomAvatarImage {
    display: block;
  }

  #braveCustomAvatarPreview .upload-spinner {
    animation: brave-custom-avatar-spin 0.75s linear infinite;
    border: 3px solid var(--leo-color-divider-subtle);
    border-radius: 50%;
    border-top-color: var(--leo-color-primary-50);
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
    background: var(--leo-color-primary-50);
    border: 2px solid var(--leo-color-page-background);
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
    font: var(--leo-font-components-button-small);
    min-height: 32px;
    padding: 0 var(--leo-spacing-l);
  }

  #braveCustomAvatarUploadBtn {
    background: var(--leo-color-button-background);
    border: none;
    color: var(--leo-color-schemes-on-primary);
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
  const image =
    root.getElementById(kCustomAvatarImageId) as HTMLImageElement | null
  if (!preview || !removeBtn || !image) {
    return
  }

  const saved = state.hasSavedAvatar
  const active = state.isActive

  // The data URL is server-built (PNG via webui::GetBitmapDataUrl) and is
  // assigned as a property — not interpolated into CSS — so there is no
  // url("...") parser context for an attacker-controlled string to escape.
  if (saved && state.dataUrl) {
    image.src = state.dataUrl
    preview.classList.remove('is-empty')
    removeBtn.hidden = false
  } else if (saved) {
    image.removeAttribute('src')
    preview.classList.add('is-empty')
    removeBtn.hidden = false
  } else {
    image.removeAttribute('src')
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

// Maximum time to wait for the `settings-manage-profile` shadow tree to
// render the custom-avatar controls before giving up and surfacing an error
// in devtools. Picked generously above any normal Polymer / lazy-load delay
// observed on slow CI hosts (~50ms typical, ~500ms p99); a longer wait is
// pointless because the row is part of the same lazy module and will not
// arrive separately.
const kBraveCustomAvatarWireTimeoutMs = 5000

// Returns true once wiring has been attached to the host's shadow tree (or
// was already wired); false when the controls aren't rendered yet so the
// caller can re-try via MutationObserver / rAF.
function tryWireBraveManageProfileCustomAvatar(host: HTMLElement): boolean {
  if ((host as unknown as Record<symbol, boolean>)[kBraveCustomAvatarWired]) {
    return true
  }
  const root = host.shadowRoot
  if (!root) {
    return false
  }

  const uploadBtn = root.getElementById(kCustomAvatarUploadBtnId)
  const removeBtn = root.getElementById(kCustomAvatarRemoveBtnId)
  const fileInput =
    root.getElementById(kCustomAvatarFileInputId) as HTMLInputElement | null
  const preview = root.getElementById(kCustomAvatarPreviewId)

  if (!uploadBtn || !removeBtn || !fileInput || !preview) {
    return false
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
      const bytes = new Uint8Array(await file.arrayBuffer())
      const result = await proxy.setCustomAvatar(bytes)
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
  return true
}

// `RegisterPolymerComponentBehaviors` is unreliable with lazy-loaded
// `manage_profile.js`. `connectedCallback` can run before the shadow tree has
// our injected nodes, and `SettingsViewMixin.ready()` runs after initial
// layout — hook both, attempt to wire immediately, and otherwise watch for
// the controls to appear instead of polling rAF on a fixed budget.
function scheduleWireBraveManageProfileCustomAvatar(host: HTMLElement) {
  queueMicrotask(() => {
    try {
      if (tryWireBraveManageProfileCustomAvatar(host)) {
        return
      }

      const startTime = performance.now()
      let observer: MutationObserver | null = null
      let timeoutHandle = 0
      let rafHandle = 0

      const cleanup = () => {
        if (observer) {
          observer.disconnect()
          observer = null
        }
        if (timeoutHandle !== 0) {
          clearTimeout(timeoutHandle)
          timeoutHandle = 0
        }
        if (rafHandle !== 0) {
          cancelAnimationFrame(rafHandle)
          rafHandle = 0
        }
      }

      const attempt = () => {
        if (tryWireBraveManageProfileCustomAvatar(host)) {
          cleanup()
          return true
        }
        return false
      }

      const ensureObserver = () => {
        const root = host.shadowRoot
        if (!root || observer) {
          return
        }
        // Observing the shadow root catches the controls being inserted
        // without burning a frame on every animation tick.
        observer = new MutationObserver(() => {
          attempt()
        })
        observer.observe(root, { childList: true, subtree: true })
      }

      // Until the host has attached its shadow root, MutationObserver can't
      // help — `attachShadow` does not fire mutations. Fall back to rAF in
      // that narrow window only; once the shadow root exists we hand off to
      // the observer.
      const tick = () => {
        rafHandle = 0
        if (attempt()) {
          return
        }
        ensureObserver()
        if (!host.shadowRoot &&
            performance.now() - startTime < kBraveCustomAvatarWireTimeoutMs) {
          rafHandle = requestAnimationFrame(tick)
        }
      }
      tick()

      timeoutHandle = window.setTimeout(() => {
        timeoutHandle = 0
        if (attempt()) {
          return
        }
        cleanup()
        // Surface the failure so it is visible in devtools / crash reports
        // instead of leaving the row inert. The upstream profile-management
        // controls are still rendered; only the upload row is unresponsive.
        console.error('[Brave Settings Overrides] Custom-avatar wiring ' +
          'timed out after ' + kBraveCustomAvatarWireTimeoutMs + 'ms; the ' +
          'upload row will not be interactive. Please reload the page.')
      }, kBraveCustomAvatarWireTimeoutMs)
    } catch (err) {
      console.warn('[Brave Settings Overrides] Custom-avatar wiring failed:',
        err)
    }
  })
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

  // Real <img> child instead of background-image: url("..."). The src is
  // assigned as a property at runtime so the data URL never enters a CSS
  // url("...") parser context.
  const image = document.createElement('img')
  image.id = kCustomAvatarImageId
  image.alt = ''
  image.setAttribute('aria-hidden', 'true')
  preview.appendChild(image)

  const placeholder = document.createElement('leo-icon')
  placeholder.className = 'placeholder'
  placeholder.setAttribute('name', 'plus-add')
  placeholder.setAttribute('aria-hidden', 'true')
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

