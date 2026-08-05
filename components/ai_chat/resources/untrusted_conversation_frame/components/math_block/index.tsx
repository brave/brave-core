// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import katex from 'katex'
// Pulls in the KaTeX font faces. style-loader injects this as a style element
// (allowed by the frame's `style-src 'unsafe-inline'`) and the woff2 files it
// references are emitted as separate resources by webpack's file-loader, which
// the generated grd picks up automatically. The frame's CSP already permits
// `font-src 'self'`. See katexCssRule in build/webpack/rules.ts, which strips
// the dead .woff/.ttf fallbacks so they aren't packaged.
import 'katex/dist/katex.css'
import styles from './style.module.scss'

/**
 * KaTeX renders by building a DOM tree and appending it — `katex.render` does
 * `node.textContent = ''` followed by `appendChild`, and there is no
 * `innerHTML` anywhere in its dist. That is what makes it usable here: the
 * untrusted conversation frame's Trusted Types policy deliberately omits
 * `createHTML` (see components/common/defaultTrustedTypesPolicy.ts), so any
 * renderer that round-trips through an HTML string would be blocked.
 */
const KATEX_OPTIONS: katex.KatexOptions = {
  // Emit MathML alongside the visual HTML so expressions are exposed to
  // assistive technology and survive copy/paste.
  output: 'htmlAndMathml',
  // Already the default, but set explicitly because this is the option that
  // disables \href, \url, \includegraphics and the \html* family. None of
  // those should ever be reachable from model output.
  trust: false,
  // Never throw. Responses stream in token by token and models emit invalid
  // LaTeX, so a bad expression has to degrade to visible source rather than
  // taking down the whole message.
  throwOnError: false,
  // KaTeX marks up an unparseable expression with an inline `color` style,
  // which a stylesheet rule could not override without `!important`. Feed it a
  // Leo token so the error state follows the theme.
  errorColor: 'var(--leo-color-systemfeedback-error-text)',
  // Don't complain about non-strict LaTeX; it would only spam the console.
  strict: false,
  // Bound adversarial input: maxExpand caps macro expansion (guarding against
  // expansion bombs) and maxSize caps user-specified dimensions, so a single
  // expression can't hang the frame or blow up the layout with something like
  // \rule{9999em}{9999em}.
  maxExpand: 1000,
  maxSize: 100,
}

function useKatex(tex: string, displayMode: boolean) {
  const ref = React.useRef<HTMLSpanElement | null>(null)

  // A layout effect so the expression is built before paint. Otherwise every
  // re-render while a response streams in would flash the raw LaTeX source.
  React.useLayoutEffect(() => {
    const element = ref.current
    if (!element) return
    try {
      katex.render(tex, element, { ...KATEX_OPTIONS, displayMode })
    } catch {
      // `throwOnError: false` handles LaTeX errors, so reaching here means
      // KaTeX itself failed. Fall back to the source text, which is more
      // useful than the empty element render() leaves behind.
      element.textContent = tex
    }
  }, [tex, displayMode])

  return ref
}

interface MathProps {
  tex: string
}

function Inline(props: MathProps) {
  const ref = useKatex(props.tex, false)
  return (
    <span
      className={styles.inline}
      ref={ref}
    />
  )
}

function Block(props: MathProps) {
  const ref = useKatex(props.tex, true)
  return (
    <span
      className={styles.block}
      ref={ref}
    />
  )
}

export default {
  Inline,
  Block,
}
