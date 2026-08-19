// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Adds a "Shortcuts" link row (hidden unless keyboard shortcut customization
// is supported) and trailing tab/window/fullscreen behavior toggles. These
// used to be added via a RegisterPolymerTemplateModifications override
// (browser/resources/settings/br/system_page.ts) before settings-system-page
// was migrated to Lit; that mechanism only works on Polymer elements, so it
// silently stopped applying. onShortcutsClick_ and shortcutsSupported_ are
// supplied by the companion chromium_src/.../system_page.ts override.
mangle((root) => {
  const section = root.querySelector('settings-section')
  if (!section) {
    throw new Error(
      `[Settings] System page: couldn't find settings-section`)
  }

  // The parent <cr-view-manager> (in brave_system_page_index.html) already
  // has this class; keeping it here too would double up the max-width/
  // margin centering it applies.
  section.classList.remove('cr-centered-card-container')

  section.insertAdjacentHTML(
    'afterbegin',
    `<cr-link-row id="shortcutsButton" class="hr"
        ?hidden="\${!this.shortcutsSupported_}"
        label="$i18n{braveShortcutsPage}"
        role-description="$i18n{subpageArrowRoleDescription}"
        @click="\${this.onShortcutsClick_}">
     </cr-link-row>`)

  // Note: browser.confirm_to_quit isn't registered as a pref outside of mac
  // (see chrome/browser/ui/cocoa/confirm_quit.cc), so the toggle for it must
  // not be mounted at all on other platforms -- a build-time "if expr" can't
  // be used here since lit_mangler runs *after* "if expr" preprocessing (see
  // tools/grit/preprocess_if_expr.py), so it'd never get stripped. A runtime
  // check (this.isMac_, from the companion system_page.ts override) with
  // Lit's own conditional templating is used instead. Its label similarly
  // can't use the usual $i18n{warnBeforeQuitting} syntax, since that string
  // isn't registered outside of mac either, and $i18n substitution happens
  // unconditionally over the whole compiled file regardless of this ternary
  // -- this.warnBeforeQuittingLabel_ (from the companion system_page.ts
  // override) is only ever read from within this isMac_-gated branch, and is
  // itself guarded so it never calls into loadTimeData on non-mac.
  section.insertAdjacentHTML(
    'beforeend',
    `<div class="hr"></div>
     <settings-toggle-button pref-key="brave.enable_closing_last_tab"
         label="$i18n{braveClosingLastTab}">
     </settings-toggle-button>
     <div class="hr"></div>
     <settings-toggle-button pref-key="brave.enable_window_closing_confirm"
         label="$i18n{braveWarnBeforeClosingWindow}">
     </settings-toggle-button>
     \${this.isMac_ ? html\`
       <div class="hr"></div>
       <settings-toggle-button pref-key="browser.confirm_to_quit"
           label="\${this.warnBeforeQuittingLabel_}">
       </settings-toggle-button>
     \` : ''}
     <div class="hr"></div>
     <settings-toggle-button pref-key="brave.show_fullscreen_reminder"
         label="$i18n{braveShowFullscreenReminder}">
     </settings-toggle-button>`)
})
