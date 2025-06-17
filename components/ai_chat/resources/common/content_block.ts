// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from './mojom'

export function createTextContentBlock(text: string): Mojom.ContentBlock {
  // We have to force typescript to accept this partial definition because the generated
  // mojom union types want undefined values for the other kinds of content blocks but the mojo
  // transport errors when these are present. It only wants a single property from the union
  // to be set, even if the value of that property is undefined.
  return {
    textContentBlock: { text }
  } as Mojom.ContentBlock
}
