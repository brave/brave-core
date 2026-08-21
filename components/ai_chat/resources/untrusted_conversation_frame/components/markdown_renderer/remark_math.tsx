// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { SKIP, visit } from 'unist-util-visit'
import type { Paragraph, Root } from 'mdast'
import type { Parent } from 'unist'
// Importing remark-math's types also pulls in mdast-util-math, whose module
// augmentation is what adds `math` and `inlineMath` to the mdast node types.
import type { Options as RemarkMathOptions } from 'remark-math'
import { loadTimeData } from '$web-common/loadTimeData'

/**
 * Kill switch for the whole feature, backed by `kAIChatMathRendering`.
 *
 * Math parsing sits on the path every assistant response takes — the remark
 * extensions run against all output and `normalizeMathDelimiters` rewrites the
 * source before parsing — so a regression here would affect all of AI Chat.
 * Reading it here, at module scope, mirrors `IS_HISTORY_FEATURE_ENABLED` in
 * untrusted_conversation_context.ts and keeps the value stable for the lifetime
 * of the frame, which matters because it feeds a module-level plugin array.
 */
export const IS_MATH_RENDERING_ENABLED = loadTimeData.getBoolean(
  'isMathRenderingEnabled',
)

// Custom element names that math nodes are rendered as. These are the only two
// tags this feature adds to the markdown renderer's element allowlist, which is
// why KaTeX output is built by a React component rather than injected into the
// tree: keeping it out of the hast means `span` and the ~30 MathML tags KaTeX
// emits never have to be allowlisted for all model output.
export const MATH_INLINE_TAG = 'mathinline'
export const MATH_BLOCK_TAG = 'mathblock'

/**
 * Options for the upstream `remark-math` plugin.
 *
 * Single-dollar text math is off because it misreads ordinary prose about
 * money: with it enabled, "costs $5 and $10 total" parses "5 and " as an
 * expression. Math is therefore delimited by `$$`, either inline (`$$x^2$$`)
 * or as a fence. `normalizeMathDelimiters` rewrites `\(…\)` and `\[…\]` into
 * that form before parsing.
 */
export const MATH_REMARK_OPTIONS: RemarkMathOptions = {
  singleDollarTextMath: false,
}

/**
 * Maps `remark-math`'s `math` / `inlineMath` nodes onto the custom elements
 * above.
 *
 * `mdast-util-math` defaults these nodes to `<pre><code class="language-math">`
 * and `<code class="language-math">`, which the markdown renderer would route
 * to the syntax-highlighted code block. Overwriting `data` replaces that
 * mapping and hands the LaTeX source through as the element's only child.
 */
export function remarkMathElements() {
  return (tree: Root) => {
    // A display equation the model wrote on a single line (`$$E=mc^2$$` as its
    // own paragraph) parses as *inline* math inside a paragraph. Promote it so
    // it renders centred in display style, as intended. Replacing the whole
    // paragraph — rather than just retyping the child — keeps the block-level
    // element from being nested inside a `<p>`.
    visit(
      tree,
      'paragraph',
      (
        node: Paragraph,
        index: number | null,
        parent: Parent | null | undefined,
      ) => {
        if (!parent || index === null || index === undefined) return
        if (node.children.length !== 1) return
        const [only] = node.children
        if (only.type !== 'inlineMath') return
        ;(parent.children as any[]).splice(index, 1, {
          type: 'math',
          value: only.value,
        })
        return SKIP
      },
    )

    visit(tree, ['math', 'inlineMath'], (node: any) => {
      node.data = {
        hName: node.type === 'math' ? MATH_BLOCK_TAG : MATH_INLINE_TAG,
        hChildren: [{ type: 'text', value: node.value }],
      }
    })
  }
}
