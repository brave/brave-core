// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const stringToKeys = (accelerator: string) =>
  // If it's an empty string, there are no keys.
  accelerator ? accelerator.split('+') : []

export const keysToString = (keys: string[]) => keys.join('+')
