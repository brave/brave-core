// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as mojom from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'

export function isLeoModel(model: mojom.Model) {
  return !!(model.options.leoModelOptions)
}
