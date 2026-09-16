/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The WebUI data source serves this page for every path under the host (see
// `SetDefaultResource()` in brave_account_ui_base.h), so the path selects the
// root element to mount.
//
// `/settings` is Android/iOS only: Settings is native there, so the account
// rows are served from here instead of being compiled into brave://settings.
function rootElementFor(pathname: string) {
  return pathname.replace(/^\/+|\/+$/g, '') === 'settings'
    ? 'brave-account-row'
    : 'brave-account-dialogs'
}

document.body.appendChild(
  document.createElement(rootElementFor(window.location.pathname)),
)
