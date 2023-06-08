// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Note: This class isn't a Polymer component because it doesn't use any fancy
 * features - it basically exists to mount our React page inside, and to not
 * let it be affected by any of the conditional templating upstairs.
 */

import { loadTimeData } from "chrome://resources/js/load_time_data.js"

// Unfortunately, our current WebPack build does not support ESModule output and
// it expects loadTimeData to be on the globalThis. The settings page has been
// migrated to use the ESModule version of loadTimeData and strings.m.js. For
// now, this provides a shim between the old and the new system.
(window as any).loadTimeData = loadTimeData

class ShortcutsPage extends HTMLElement {
  connectedCallback() {
    this.attachShadow({ mode: 'open' })


    import('/commands.bundle.js' as any)
      .then(() => (window as any).mountCommands(this.shadowRoot))
      .catch(() => {})
  }
}

customElements.define('settings-shortcuts-page', ShortcutsPage)
