// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { loadTimeData } from "chrome://resources/js/load_time_data.js"
import { RemoteMappingService } from "./remote_mapping_service.js"

// Unfortunately, our current WebPack build does not support ESModule output and
// it expects loadTimeData to be on the globalThis. The settings page has been
// migrated to use the ESModule version of loadTimeData and strings.m.js. For
// now, this provides a shim between the old and the new system.
(window as any).loadTimeData = loadTimeData

class EmailAliasesPage extends HTMLElement {
  connectedCallback() {
    this.attachShadow({ mode: 'open' })
    import('/email_aliases.bundle.js' as any)
      .then(() => (window as any).mountEmailAliases(this.shadowRoot, new RemoteMappingService))
      .catch(() => {})
  }
}

customElements.define('settings-email-aliases-page', EmailAliasesPage)
