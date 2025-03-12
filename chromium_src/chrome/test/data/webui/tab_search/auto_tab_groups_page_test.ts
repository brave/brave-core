// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This override is needed to avoid build time TS error from
// upstream unit_tests target complaining some child components might be null
// because we do not have them in our implementation.
// We replaced the whole auto_tab_groups_page from upstream, so upstream test
// here is not relevant to us. Currently we do not run TabSearchFocusTest
// which runs tests for auto_tab_groups_page either. It should be fairly safe to
// ignore this file.
