// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export function InferControlsFromArgs (args: { [key: string]: any }) {
  // If type is boolean or string then use those simple controls
  const simpleEntries = Object.entries(args).filter(
    ([key, value]) => typeof value === 'boolean' || typeof value === 'string')
  const simpleControls = simpleEntries.map(
    ([key, value]) => [key, {
      control: typeof value === 'boolean' ? 'boolean' : 'text'
    }])
  return Object.fromEntries(simpleControls)
}
