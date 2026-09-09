/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '../controls/settings_checkbox.js'

import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from 'chrome://resources/js/load_time_data.js'

import type { SettingsCheckboxElement } from '../controls/settings_checkbox.js'
import type { BrowserProfile } from '../people_page/import_data_browser_proxy.js'
import { SettingsImportDataDialogElement } from '../people_page/import_data_dialog.js'

// Brave imports extensions and payment methods on top of the upstream data
// types. The matching browser support flags and pref keys come from
// chromium_src/chrome/browser/ui/webui/settings/import_data_handler.cc.
// NOTE: when adding a new import type here, also consider adding it to the
// welcome page import flow.
const kBraveImportTypes = [
  {
    id: 'importDialogExtensions',
    prefKey: 'import_dialog_extensions',
    labelId: 'importExtensions',
    supportedBy: 'extensions',
  },
  {
    id: 'importDialogPayments',
    prefKey: 'import_dialog_payments',
    labelId: 'importPayments',
    supportedBy: 'payments',
  },
] as const

// `selected_` is `protected`, and its Brave-only fields are added by
// chromium_src, so give this file its own view of the prototype rather than
// intersecting with SettingsImportDataDialogElement.
interface PatchableImportDataDialog {
  shadowRoot: ShadowRoot | null
  selected_: BrowserProfile & { extensions?: boolean; payments?: boolean }
}

const proto = SettingsImportDataDialogElement.prototype as unknown as
  PatchableImportDataDialog & {
    updated: (changedProperties: PropertyValues) => void
  }

function createCheckbox(
  importType: (typeof kBraveImportTypes)[number],
): SettingsCheckboxElement {
  const checkbox = document.createElement('settings-checkbox')
  checkbox.id = importType.id
  checkbox.prefKey = importType.prefKey
  checkbox.label = loadTimeData.getString(importType.labelId)
  // The dialog commits the prefs itself, once the import actually starts.
  checkbox.noSetPref = true
  return checkbox
}

function syncBraveImportTypes(dialog: PatchableImportDataDialog) {
  const root = dialog.shadowRoot
  if (!root) {
    return
  }

  // Upstream keeps the checkboxes in a parent of their own, separate from the
  // browser select, to avoid confusing screen readers. Keep Brave's there too.
  const container = root.getElementById('importDialogHistory')?.parentElement
  if (!container) {
    console.error('[Settings] Missing import data dialog checkbox container')
    return
  }

  for (const importType of kBraveImportTypes) {
    let checkbox = root.getElementById(
      importType.id,
    ) as SettingsCheckboxElement | null
    if (!checkbox) {
      checkbox = createCheckbox(importType)
      container.appendChild(checkbox)
    }
    checkbox.hidden = !dialog.selected_[importType.supportedBy]
  }
}

const originalUpdated = proto.updated
proto.updated = function (
  this: PatchableImportDataDialog,
  changedProperties: PropertyValues,
) {
  // Sync before the original `updated()`, since that counts the visible
  // checked checkboxes to decide whether importing is possible at all.
  syncBraveImportTypes(this)
  originalUpdated.call(this, changedProperties)
}
