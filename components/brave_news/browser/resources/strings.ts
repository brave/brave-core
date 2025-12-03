// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$web-common/strings'
import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import { BraveNewsStrings } from 'gen/components/grit/brave_components_webui_strings'

declare global {
  // Expose the BraveNewsStrings enum on the global `S` enum.
  interface Strings {
    BraveNewsStrings: typeof BraveNewsStrings
  }
}

export function getString(key: keyof typeof BraveNewsStrings) {
  return loadTimeData.getString(key)
}
