// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Centralised augmentation for window globals that Brave settings code
// assigns to. Kept in an unconditionally-built file so the augmentation is
// present regardless of which optional features (wallet, tor, etc.) are
// enabled in the build.
declare global {
  interface Window {
    testing: Record<string, unknown>;
    loadTimeData: unknown;
  }
}

export {};
