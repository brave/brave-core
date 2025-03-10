/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Store } from '../lib/store'

import { BackgroundState, BackgroundActions } from '../models/backgrounds'

export function initializeBackgrounds(
  store: Store<BackgroundState>
) : BackgroundActions {
  return {
    setBackgroundsEnabled(enabled) {},
    setSponsoredImagesEnabled(enabled) {},
    selectBackground(type, value) {},
    async showCustomBackgroundChooser() { return false },
    async removeCustomBackground(background) {},
    notifySponsoredImageLogoClicked() {},
    notifySponsoredRichMediaEvent(type) {}
  }
}
