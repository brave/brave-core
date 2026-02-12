// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BRAVE_SUMMARY_MODEL_KEY } from '../common/constants'

import * as Mojom from '../common/mojom'

const HIDDEN_MODEL_KEYS = new Set<string>([BRAVE_SUMMARY_MODEL_KEY])

export function isLeoModel(model: Mojom.Model) {
  return !!model.options.leoModelOptions
}

export function isSelectableModel(model: Mojom.Model) {
  return !HIDDEN_MODEL_KEYS.has(model.key)
}
