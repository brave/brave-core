/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The WebUI data source serves this page for every path under the host (see
// `SetDefaultResource` in brave_account_ui_base.h), so the route is read here
// and the matching root element is mounted.
//
// `/settings` is Android/iOS only: desktop reaches the same element through the
// brave://settings bundle, and never navigates here.
//
// A bare path maps to the authentication flows, so existing entry points that
// navigate to chrome://account/ keep working.
function rootElementFor(pathname: string) {
  switch (pathname.replace(/^\/+|\/+$/g, '')) {
    case 'settings':
      return 'brave-account-settings'
    // `authentication` and the bare path mount the same flows. The bare path is
    // what every existing entry point navigates to, but the account navigation
    // throttle cancels navigations to it that aren't AUTO_TOPLEVEL, so in-page
    // navigation out of `settings` has to use the named route.
    case 'authentication':
    default:
      return 'brave-account-dialogs'
  }
}

document.body.appendChild(
  document.createElement(rootElementFor(window.location.pathname)),
)
