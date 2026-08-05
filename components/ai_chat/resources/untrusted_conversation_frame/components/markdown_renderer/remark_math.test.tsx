// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import remarkGfm from 'remark-gfm'
import remarkParse from 'remark-parse'
import remarkMath from 'remark-math'
import { unified } from 'unified'
import { visit } from 'unist-util-visit'
import type { Root } from 'mdast'
import { allowedElements } from './index'
import {
  MATH_BLOCK_TAG,
  MATH_INLINE_TAG,
  MATH_REMARK_OPTIONS,
  remarkMathElements,
} from './remark_math'

const runPlugin = (tree: Root) => {
  remarkMathElements()(tree)
  return tree as any
}

describe('remarkMathElements', () => {
  it('maps a block math node onto the math block element', () => {
    const tree = runPlugin({
      type: 'root',
      children: [{ type: 'math', value: 'x^2' } as any],
    })

    expect(tree.children[0].data).toEqual({
      hName: MATH_BLOCK_TAG,
      hChildren: [{ type: 'text', value: 'x^2' }],
    })
  })

  it('maps an inline math node onto the math inline element', () => {
    const tree = runPlugin({
      type: 'root',
      children: [
        {
          type: 'paragraph',
          children: [
            { type: 'text', value: 'a ' },
            { type: 'inlineMath', value: 'x^2' } as any,
          ],
        },
      ],
    })

    expect(tree.children[0].children[1].data).toEqual({
      hName: MATH_INLINE_TAG,
      hChildren: [{ type: 'text', value: 'x^2' }],
    })
  })

  // mdast-util-math defaults these nodes to `<pre><code class="language-math">`
  // / `<code class="language-math">`, which the renderer would otherwise route
  // to the syntax-highlighted code block.
  it('overrides the default code element mapping', () => {
    const tree = runPlugin({
      type: 'root',
      children: [
        {
          type: 'math',
          value: 'x^2',
          data: {
            hName: 'pre',
            hChildren: [{ type: 'element', tagName: 'code', children: [] }],
          },
        } as any,
      ],
    })

    expect(tree.children[0].data.hName).toBe(MATH_BLOCK_TAG)
    expect(tree.children[0].data.hChildren).toEqual([
      { type: 'text', value: 'x^2' },
    ])
  })

  it('promotes a paragraph holding only inline math to block math', () => {
    const tree = runPlugin({
      type: 'root',
      children: [
        {
          type: 'paragraph',
          children: [{ type: 'inlineMath', value: 'E=mc^2' } as any],
        },
      ],
    })

    expect(tree.children).toHaveLength(1)
    expect(tree.children[0].type).toBe('math')
    expect(tree.children[0].data.hName).toBe(MATH_BLOCK_TAG)
  })

  it('does not promote inline math that sits alongside text', () => {
    const tree = runPlugin({
      type: 'root',
      children: [
        {
          type: 'paragraph',
          children: [
            { type: 'inlineMath', value: 'x' } as any,
            { type: 'text', value: ' is small' },
          ],
        },
      ],
    })

    expect(tree.children[0].type).toBe('paragraph')
    expect(tree.children[0].children[0].data.hName).toBe(MATH_INLINE_TAG)
  })

  it('leaves trees without math untouched', () => {
    const tree = runPlugin({
      type: 'root',
      children: [
        { type: 'paragraph', children: [{ type: 'text', value: 'hello' }] },
      ],
    })

    expect(tree.children[0].type).toBe('paragraph')
    expect(tree.children[0].data).toBeUndefined()
  })
})

describe('math markdown pipeline', () => {
  // Mirrors the renderer's remark plugin chain, so these assertions cover what
  // MarkdownRenderer actually produces for a given piece of model output.
  const processor = unified()
    .use(remarkParse)
    .use(remarkGfm)
    .use(remarkMath, MATH_REMARK_OPTIONS)
    .use(remarkMathElements)

  // Returns the element name each math node will be rendered as, in order.
  const mathTagsIn = (markdown: string): string[] => {
    const tree = processor.runSync(processor.parse(markdown))
    const tags: string[] = []
    visit(tree, (node: any) => {
      if (node.data?.hName === MATH_INLINE_TAG) tags.push(MATH_INLINE_TAG)
      if (node.data?.hName === MATH_BLOCK_TAG) tags.push(MATH_BLOCK_TAG)
    })
    return tags
  }

  const texIn = (markdown: string): string[] => {
    const tree = processor.runSync(processor.parse(markdown))
    const values: string[] = []
    visit(tree, (node: any) => {
      if (node.type === 'math' || node.type === 'inlineMath') {
        values.push(node.value)
      }
    })
    return values
  }

  it('parses inline $$...$$ within a sentence as inline math', () => {
    expect(mathTagsIn('The area is $$\\pi r^2$$ exactly.')).toEqual([
      MATH_INLINE_TAG,
    ])
    expect(texIn('The area is $$\\pi r^2$$ exactly.')).toEqual(['\\pi r^2'])
  })

  it('parses a $$ fence as block math', () => {
    expect(mathTagsIn('Before\n\n$$\nE=mc^2\n$$\n\nAfter')).toEqual([
      MATH_BLOCK_TAG,
    ])
    expect(texIn('Before\n\n$$\nE=mc^2\n$$\n\nAfter')).toEqual(['E=mc^2'])
  })

  it('parses a single-line $$...$$ paragraph as block math', () => {
    expect(mathTagsIn('Before\n\n$$E=mc^2$$\n\nAfter')).toEqual([
      MATH_BLOCK_TAG,
    ])
  })

  // Single-dollar text math is disabled precisely so this does not happen.
  it('does not treat prose about prices as math', () => {
    expect(mathTagsIn('It costs $5 and $10 in total.')).toEqual([])
  })

  it('leaves math delimiters inside a fenced code block as code', () => {
    expect(mathTagsIn('```tex\n$$x^2$$\n```')).toEqual([])
  })

  it('leaves math delimiters inside inline code alone', () => {
    expect(mathTagsIn('use `$$x^2$$` here')).toEqual([])
  })

  it('handles several expressions in one document', () => {
    expect(mathTagsIn('$$a$$ and $$b$$ then\n\n$$\nc\n$$\n\ndone')).toEqual([
      MATH_INLINE_TAG,
      MATH_INLINE_TAG,
      MATH_BLOCK_TAG,
    ])
  })

  it('keeps inline math inside a list item inline', () => {
    expect(mathTagsIn('- an item with $$x^2$$ in it')).toEqual([
      MATH_INLINE_TAG,
    ])
  })
})

describe('markdown renderer allowlist', () => {
  it('permits the math elements', () => {
    expect(allowedElements).toContain(MATH_INLINE_TAG)
    expect(allowedElements).toContain(MATH_BLOCK_TAG)
  })

  // KaTeX output is built by a React component rather than injected into the
  // tree, so none of the tags it emits need to be reachable from model output.
  it('does not permit raw span or MathML elements', () => {
    for (const tag of ['span', 'math', 'semantics', 'mrow', 'annotation']) {
      expect(allowedElements).not.toContain(tag)
    }
  })
})

describe('kAIChatMathRendering kill switch', () => {
  // `IS_MATH_RENDERING_ENABLED` is read once at module scope, so the module
  // graph has to be re-evaluated against a different loadTimeData value.
  const loadAllowlistWithMathRendering = async (enabled: boolean) => {
    const windowAsAny = window as any
    const original = windowAsAny.loadTimeData
    windowAsAny.loadTimeData = {
      ...original,
      getBoolean: (key: string) =>
        key === 'isMathRenderingEnabled' ? enabled : original.getBoolean(key),
    }
    try {
      let allowlist: string[] = []
      await jest.isolateModulesAsync(async () => {
        allowlist = (await import('./index')).allowedElements
      })
      return allowlist
    } finally {
      windowAsAny.loadTimeData = original
    }
  }

  it('closes the allowlist when the feature is disabled', async () => {
    const allowlist = await loadAllowlistWithMathRendering(false)

    expect(allowlist).not.toContain(MATH_INLINE_TAG)
    expect(allowlist).not.toContain(MATH_BLOCK_TAG)
    // The rest of the allowlist is untouched.
    expect(allowlist).toContain('colorchip')
  })

  it('opens the allowlist when the feature is enabled', async () => {
    const allowlist = await loadAllowlistWithMathRendering(true)

    expect(allowlist).toContain(MATH_INLINE_TAG)
    expect(allowlist).toContain(MATH_BLOCK_TAG)
  })
})
