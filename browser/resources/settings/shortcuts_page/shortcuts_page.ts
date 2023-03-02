// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

 /**
  * Note: This class isn't a Polymer component because it doesn't use any fancy
  * features - it basically exists to mount our React page inside, and to not
  * let it be affected by any of the conditional templating upstairs.
  */
class ShortcutsPage extends HTMLElement {
  connectedCallback() {
    this.attachShadow({ mode: 'open' })
    import("/commands.bundle.js" as any)
      .then(() => (window as any).mountCommands(this.shadowRoot))
  }
}

customElements.define('settings-shortcuts-page', ShortcutsPage);
