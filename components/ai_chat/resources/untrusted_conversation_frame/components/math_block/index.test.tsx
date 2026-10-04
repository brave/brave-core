// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { render } from '@testing-library/react'
import * as React from 'react'
import Math from './index'

describe('math_block Inline', () => {
  it('renders an expression with KaTeX', () => {
    const { container } = render(<Math.Inline tex='\pi r^2' />)

    expect(container.querySelector('.katex')).not.toBeNull()
    expect(container.querySelector('.katex-display')).toBeNull()
  })

  it('keeps the LaTeX source in a MathML annotation for copy and a11y', () => {
    const { container } = render(<Math.Inline tex='x^2' />)

    expect(container.querySelector('annotation')?.textContent).toBe('x^2')
    expect(container.querySelector('math')).not.toBeNull()
  })

  it('re-renders when the expression changes', () => {
    const { container, rerender } = render(<Math.Inline tex='x^2' />)
    rerender(<Math.Inline tex='y^3' />)

    expect(container.querySelector('annotation')?.textContent).toBe('y^3')
    // The previous expression must not be left behind alongside the new one.
    expect(container.querySelectorAll('.katex')).toHaveLength(1)
  })
})

describe('math_block Block', () => {
  it('renders in display mode', () => {
    const { container } = render(<Math.Block tex='E=mc^2' />)

    expect(container.querySelector('.katex-display')).not.toBeNull()
  })
})

describe('math_block error handling', () => {
  it('shows invalid LaTeX as source rather than throwing', () => {
    const { container } = render(<Math.Inline tex='\frac{1}{' />)

    // `throwOnError: false` makes KaTeX mark the expression instead of raising.
    const error = container.querySelector('.katex-error')
    expect(error).not.toBeNull()
    expect(error?.textContent).toBe('\\frac{1}{')
  })

  it('renders an empty expression without throwing', () => {
    expect(() => render(<Math.Inline tex='' />)).not.toThrow()
  })
})

describe('math_block hardening', () => {
  // `trust: false` is what disables \href, \url, \includegraphics and the
  // \html* family, none of which should be reachable from model output.
  it('does not produce a link from \\href', () => {
    const { container } = render(
      <Math.Inline tex='\href{https://evil.example}{click}' />,
    )

    expect(container.querySelector('a')).toBeNull()
    expect(container.querySelector('[href]')).toBeNull()
    // The command is rendered as its literal name instead of being honoured.
    // The URL still appears in the MathML annotation, but only as inert
    // source text.
    expect(container.querySelector('.katex-html')?.textContent).toBe('\\href')
  })

  it('does not produce an image from \\includegraphics', () => {
    const { container } = render(
      <Math.Inline tex='\includegraphics{https://evil.example/x.png}' />,
    )

    expect(container.querySelector('img')).toBeNull()
  })

  it('does not apply author-supplied classes from \\htmlClass', () => {
    const { container } = render(<Math.Inline tex='\htmlClass{evil}{x}' />)

    expect(container.querySelector('.evil')).toBeNull()
  })

  it('caps sizes so a single expression cannot blow up the layout', () => {
    const { container } = render(<Math.Inline tex='\rule{9999em}{9999em}' />)

    // maxSize clamps the requested 9999em down to the configured limit.
    const rule = container.querySelector<HTMLElement>('.katex-rule')
    expect(rule).not.toBeNull()
    expect(rule!.style.borderRightWidth).toBe('100em')
  })

  /**
   * Builds `\def\ma{\mb\mb}\def\mb{\mc\mc}…\def\m?{x}\ma`, where each macro
   * expands to two copies of the next one. Expanding `\ma` therefore costs
   * `2^0 + 2^1 + … + 2^levels` = `2^(levels+1) - 1` expansions, which is what
   * KaTeX counts against `maxExpand`.
   */
  function buildExpansionBomb(levels: number) {
    const name = (i: number) => `\\m${String.fromCharCode(97 + i)}`
    const defs = Array.from(
      { length: levels },
      (_unused, i) => `\\def${name(i)}{${name(i + 1)}${name(i + 1)}}`,
    )
    return `${defs.join('')}\\def${name(levels)}{x}${name(0)}`
  }

  it('renders a macro chain that stays within the expansion budget', () => {
    // 2^6 - 1 = 63 expansions, comfortably under maxExpand.
    const { container } = render(<Math.Inline tex={buildExpansionBomb(5)} />)

    expect(container.querySelector('.katex-error')).toBeNull()
    expect(container.querySelector('.katex')).not.toBeNull()
  })

  it('aborts an expansion bomb instead of hanging', () => {
    // 2^11 - 1 = 2047 expansions, which blows the 1000 budget. Without
    // maxExpand this would keep doubling until it exhausted memory.
    const bomb = buildExpansionBomb(10)

    const { container } = render(<Math.Inline tex={bomb} />)

    // KaTeX raises a ParseError once the budget is spent, and
    // `throwOnError: false` turns that into the visible error state rather
    // than letting it escape into React.
    expect(container.querySelector('.katex-error')).not.toBeNull()
  })
})
