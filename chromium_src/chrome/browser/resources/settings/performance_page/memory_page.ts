// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Upstream's memory_page.ts doesn't import the tab-discard exception list
// (it lives on the performance page instead, which we don't show); the
// companion memory_page.html.ts.lit_mangler.ts override injects
// <tab-discard-exception-list> into this page's template, so it needs to be
// registered here.
import './tab_discard/exception_list.js'

export * from './memory_page-chromium.js'
