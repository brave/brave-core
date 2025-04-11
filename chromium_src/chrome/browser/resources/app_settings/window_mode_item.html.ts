// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from '//resources/lit/v3_0/lit.rollup.js';
import type { WindowModeItemElement } from './window_mode_item.js';
import { WindowMode } from '//resources/cr_components/app_management/constants.js';
import { BrowserProxy } from '//resources/cr_components/app_management/browser_proxy.js';
import { loadTimeData } from '//resources/js/load_time_data.js';

export function getHtml(this: WindowModeItemElement) {
  const handler = (e: { value: string }) => {
    BrowserProxy.getInstance().handler.setWindowMode(this.app.id, parseInt(e.value) as WindowMode);
  }
  return html`<!--_html_template_start_-->
<div ?hidden=${this.app.hideWindowMode}>
  <div class="permission-section-header">
    <div class="header-text">${loadTimeData.getString('appManagementOpenModeLabel')}</div>
  </div>
  <div class="permission-list indented-permission-block">
    <leo-radiobutton class="subpermission-row" name="window-mode" @change=${handler} value="${WindowMode.kWindow}" currentValue=${this.app.windowMode}>
      ${loadTimeData.getString('appManagementWindowModeLabel')}
    </leo-radiobutton>
    <leo-radiobutton class="subpermission-row" name="window-mode" @change=${handler} value="${WindowMode.kBrowser}" currentValue=${this.app.windowMode}>
      ${loadTimeData.getString('appManagementBrowserModeLabel')}
    </leo-radiobutton>
    ${loadTimeData.getBoolean('isPWAsTabStripSettingsEnabled') ? html`<leo-radiobutton class="subpermission-row" name="window-mode" @change=${handler} value="${WindowMode.kTabbedWindow}" currentValue=${this.app.windowMode}>
      ${loadTimeData.getString('appManagementTabbedWindowModeLabel')}
    </leo-radiobutton>` : ''}
  </div>
</div>
<!--_html_template_end_-->`;
}
