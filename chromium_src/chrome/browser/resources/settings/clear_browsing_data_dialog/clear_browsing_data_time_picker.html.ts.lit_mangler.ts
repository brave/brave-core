// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Brave puts the check after the time period's label, and uses Leo's icon for
// it. This used to be done with a RegisterPolymerTemplateModifications override
// (browser/resources/settings/br/clear_browsing_data_time_picker.ts) before
// settings-clear-browsing-data-time-picker was migrated to Lit; that mechanism
// only works on Polymer elements, so it silently stopped applying.
//
// Only this nested template is mangled. The root template must be left alone:
// it holds a nested html`` template inside cr-lazy-render-lit's `.template`
// attribute, and the quotes in there terminate the attribute value as far as
// the mangler's HTML parser is concerned, so round-tripping the root drops the
// nested </cr-action-menu> and escapes the tag it belongs to. The companion
// clear_browsing_data_time_picker.ts override picks up what would otherwise
// have been a root-level change.
mangle((root) => {
  const chip = root.querySelector('.time-period-chip')
  const icon = chip?.querySelector('cr-icon')
  if (!icon) {
    throw new Error(
      `[Settings] Time picker: couldn't find the time period chip's icon`)
  }
  icon.setAttribute('icon', 'check-circle-outline')
  chip!.appendChild(icon)
}, (t) => t.text.includes('time-period-chip'))
