// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler';

mangle((element) => {
  const toggleRow = element.querySelector('app-management-toggle-row')
  if (!toggleRow) {
    throw new Error('[Brave App Settings] Could not find app-management-toggle-row. Maybe upstream has changed to a radio button?')
  }
  toggleRow.outerHTML = `<div ?hidden=\${this.app.hideWindowMode}>
    <div class="permission-section-header">
      <div class="header-text">$i18n{appManagementOpenModeLabel}</div>
    </div>
    <div class="permission-list indented-permission-block">
      \${this.windowModes.map(mode => html\`<leo-radiobutton class="subpermission-row" name="window-mode" @change=\${this.onChange} value="\${mode.value}" currentValue=\${this.app.windowMode}>
        \${mode.label}
      </leo-radiobutton>\`)}
    </div>
  </div>`
}, t => t.text.includes('id="toggle-row"'))
