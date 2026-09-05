// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { PrefService } from '/shared/settings/prefs2/pref_service.js'

// tools/polymer/html_to_wrapper.py's `detect_template_type` decides whether
// settings_checkbox.html compiles to a real Polymer <template> (needed by
// Polymer's _prepareTemplate/_stampTemplate) or a plain getTrustedHTML
// string, by text-searching *this* file (the "definition file" it pairs
// with settings_checkbox.html) for a literal reference to the path
// 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js' vs one
// to lit.rollup.js. Since this override doesn't otherwise import anything
// from Polymer directly, this comment's mention of that path is load-
// bearing: without it, detection falls back to a getTrustedHTML wrapper
// instead of an HTMLTemplateElement, and settings-checkbox blows up with
// "t.cloneNode is not a function" the first time it's instantiated.

import { SettingsCheckboxElement } from './settings_checkbox-chromium.js'

declare module './settings_checkbox-chromium.js' {
  interface SettingsCheckboxElement {
    prefKey?: string
    validatePref_(): void
  }
}

// Upstream hasn't migrated settings-checkbox to the pref-key/PrefService
// mechanism yet (unlike settings-toggle-button, settings-dropdown-menu,
// settings-radio-group and controlled-radio-button, which all already
// support it). Add the same support here so checkboxes can be used on pages
// that no longer thread a cascading `prefs` object down (e.g. Lit-migrated
// pages).
//
// This is a plain prototype patch rather than composing PrefKeyObserverMixin
// via a subclass (as settings_toggle_button.ts does natively upstream):
// `prefKey` isn't referenced by settings_checkbox.html, so it doesn't need
// to be a reactive Polymer property -- reading it once from the attribute
// is sufficient, and it keeps this override simpler.

// PrefControlMixin's validatePref_() (called from connectedCallback, and
// again as an observer any time `pref` changes) only skips its "Pref error
// [not found]" console.error when `this.prefKey` is already truthy.
// Checking the `pref-key` attribute directly instead is more robust than
// depending on `this.prefKey` having been set by a particular point in
// Polymer's internal lifecycle ordering.
const originalValidatePref = SettingsCheckboxElement.prototype.validatePref_
SettingsCheckboxElement.prototype.validatePref_ = function(
    this: SettingsCheckboxElement) {
  if (this.hasAttribute('pref-key')) {
    return
  }
  originalValidatePref.call(this)
}

// Read `pref-key` and start mirroring the pref in `ready()`, *not*
// connectedCallback. connectedCallback (along with disconnectedCallback,
// adoptedCallback and attributeChangedCallback) is one of the four
// lifecycle callbacks the Custom Elements spec reads off the prototype and
// caches at `customElements.define()` time -- patching it afterwards (as
// this file necessarily does, since customElements.define() already runs
// inside settings_checkbox-chromium.js by the time this file's own code
// executes) has no effect: the browser keeps invoking the cached,
// pre-patch version forever. `ready()` isn't part of that spec; it's a
// Polymer-internal convention invoked via an ordinary `this.ready()` call
// from inside the (cached) native connectedCallback, so it's still resolved
// dynamically and patching it here works correctly. It's also only ever
// called once per instance, so there's no need to guard against
// re-registering an observer on reconnect.
const originalReady = SettingsCheckboxElement.prototype.ready
SettingsCheckboxElement.prototype.ready = function(
    this: SettingsCheckboxElement) {
  originalReady.call(this)

  const prefKey = this.getAttribute('pref-key')
  if (prefKey) {
    this.prefKey = prefKey
    // Use `this.set(...)` rather than a plain assignment: `checked` depends
    // on the sub-path observer `prefValueChanged_(pref.value)`, and that
    // only reliably re-fires on a wholesale `pref` reassignment via
    // Polymer's `set()` API (the same pattern PrefServiceObserverMixin's
    // own mirrorPref() uses).
    PrefService.getInstance().addObserver(
        prefKey, pref => { this.set('pref', pref) })
  }
}

const originalSendPrefChangeInternal =
    SettingsCheckboxElement.prototype.sendPrefChangeInternal
SettingsCheckboxElement.prototype.sendPrefChangeInternal = function(
    this: SettingsCheckboxElement, value: boolean|number) {
  if (this.prefKey) {
    PrefService.getInstance().setPrefValue(this.prefKey, value)
    return
  }

  // Fallback to the old 'prefs' mechanism if this element hasn't been
  // migrated to use pref-key yet.
  originalSendPrefChangeInternal.call(this, value)
}

export * from './settings_checkbox-chromium.js'
