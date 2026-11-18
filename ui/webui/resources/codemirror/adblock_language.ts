// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Placeholder tokenizer for adblock filters until tokenization is delegated
// to adblock-rust. Currently only highlights `!` comment lines.

import { HighlightStyle, StreamLanguage, syntaxHighlighting } from '@codemirror/language'
import { tags } from '@lezer/highlight'

const adblockLanguage = StreamLanguage.define({
  token(stream) {
    if (stream.sol() && stream.peek() === '!') {
      stream.skipToEnd()
      return 'comment'
    }
    stream.skipToEnd()
    return null
  },
})

// The color is a custom property defined in <brave-code-editor>'s shadow
// root, which switches with the settings page theme.
const adblockHighlighting = HighlightStyle.define([
  { tag: tags.comment, color: 'var(--brave-code-comment-color)' },
])

export const adblockFilterExtensions = [
  adblockLanguage,
  syntaxHighlighting(adblockHighlighting),
]
