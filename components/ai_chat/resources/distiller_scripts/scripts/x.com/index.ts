/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LEO_DISTILLATION_LEVEL } from '../distillation'
import distill from './distiller'

window.addEventListener('{initiate_key}', (event: Event) => {
  const level =
    event instanceof CustomEvent && event.detail.level > -1
      ? event.detail.level
      : LEO_DISTILLATION_LEVEL.FULL

  const type = '{complete_key}'
  const result = distill(level)

  window.dispatchEvent(new CustomEvent(type, { detail: { result } }))
})
