// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getModelIcon } from './constants'
import * as Mojom from './mojom'

describe('getModelIcon', () => {
  it('Should return fallback icon for unknown model keys', () => {
    const unknownModel = { key: 'unknown-model' } as Mojom.Model
    const emptyModel = { key: '' } as Mojom.Model
    const invalidModel = { key: 'chat-invalid' } as Mojom.Model

    expect(getModelIcon(unknownModel)).toBe('product-brave-leo')
    expect(getModelIcon(emptyModel)).toBe('product-brave-leo')
    expect(getModelIcon(invalidModel)).toBe('product-brave-leo')
  })

  it('Should return ollama icon for Ollama models', () => {
    const ollamaModel = {
      key: 'custom-model',
      options: {
        customModelOptions: {
          endpoint: {
            url: Mojom.OLLAMA_ENDPOINT,
          },
        },
      },
    } as Mojom.Model

    expect(getModelIcon(ollamaModel)).toBe('ollama')
  })
})
