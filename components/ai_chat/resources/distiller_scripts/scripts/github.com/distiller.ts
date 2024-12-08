/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LEO_DISTILLATION_LEVEL } from '../distillation'
import { distillBranches } from './branches'
import { GetPageType, SupportedPage } from './utils'

let _DISTILLATION_LEVEL = LEO_DISTILLATION_LEVEL.LOW

export function getDistillationLevel() {
  return _DISTILLATION_LEVEL
}

export default function distill(level: LEO_DISTILLATION_LEVEL) {
  _DISTILLATION_LEVEL = level

  switch (GetPageType(document)) {
    case SupportedPage.BRANCHES:
      return distillBranches()
    default:
      return null
  }
}
