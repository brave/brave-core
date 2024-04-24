/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Markdown from 'react-markdown'
import { Root, Element, Properties } from 'hast'
const visit = require('unist-util-visit/index')

import styles from './style.module.scss'
import CaretSVG from '../svg/caret'

const CodeBlock = React.lazy(async () => ({
  default: (await import('../code_block')).default.Block
}))
const CodeInline = React.lazy(async () => ({
  default: (await import('../code_block')).default.Inline
}))

// Any custom prop on DOM element must be a lowercase otherwise React will utilize it. The following gives hint:
// Warning: React does not recognize the `%s` prop on a DOM element. If you intentionally want it to appear in the DOM as a custom attribute, spell it as lowercase `%s` instead.

interface ElementWithData extends Element {
  properties: Properties & {
    'is-cursor-visible': string
  }
}

const allowedElements = [
  // Headings
  'h1',
  'h2',
  'h3',
  'h4',
  'h5',
  'h6',

  // Text formatting
  'blockquote',
  'code',
  'del',
  'em',
  'strong',
  'sup',

  // Lists
  'li',
  'ol',
  'ul',

  // Structural elements
  'p',
  'pre',
  'section',

  // Line elements
  'br',
  'hr'
]

interface MarkdownRendererProps {
  text: string
  shouldShowTextCursor: boolean
}

export default function MarkdownRenderer(mainProps: MarkdownRendererProps) {
  const plugin = React.useCallback(() => {
    const transformer = (tree: Root) => {
      const totalLines = tree.position?.end.line

      visit(tree, 'element', (el: Element) => {
        if (!('properties' in el)) {
          el.properties = {}
        }

        const newEl = el as ElementWithData

        newEl.properties = {
          ...el.properties,
          'is-cursor-visible': (totalLines === el.position?.end.line).toString()
        }
      })
    }

    return transformer
  }, [])

  return (
    <div className={styles.markdownContainer}>
      <Markdown
        allowedElements={allowedElements}
        // We only read the total lines value from AST
        // if the component is allowed to show the text cursor.
        rehypePlugins={mainProps.shouldShowTextCursor ? [plugin] : undefined}
        unwrapDisallowed={true}
        children={mainProps.text}
        components={{
          p: (props) => {
            const el = props.node as ElementWithData
            const isCursorVisible =
              el.properties['is-cursor-visible'] === 'true'

            return (
              <p>
                {props.children}
                {isCursorVisible && (
                  <span className={styles.textCursor}>
                    <CaretSVG />
                  </span>
                )}
              </p>
            )
          },
          code: (props) => {
            const { children, className } = props
            const match = /language-([^ ]+)/.exec(className || '')
            return match ? (
              <React.Suspense fallback={'...'}>
                <CodeBlock
                  lang={match[1]}
                  code={String(children).replace(/\n$/, '')}
                />
              </React.Suspense>
            ) : (
              <React.Suspense fallback={'...'}>
                <CodeInline code={String(children)} />
              </React.Suspense>
            )
          }
        }}
      />
    </div>
  )
}
