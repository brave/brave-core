/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {Model, OLLAMA_ENDPOINT} from './brave_leo_assistant_browser_proxy.js'
import {modelIcons, fallbackModelIcon} from '../model_icon_map.js'

export function getModelIcon(model: Model): string {
  if (model.options?.customModelOptions?.endpoint === OLLAMA_ENDPOINT) {
    return 'ollama'
  }
  return modelIcons[model.key] ?? fallbackModelIcon
}
