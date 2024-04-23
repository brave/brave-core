/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Markdown from 'react-markdown'

import styles from './style.module.scss'
import CaretSVG from '../svg/caret'

const CodeBlock = React.lazy(async () => ({
  default: (await import('../code_block')).default.Block
}))
const CodeInline = React.lazy(async () => ({
  default: (await import('../code_block')).default.Inline
}))

interface MarkdownRendererProps {
  text: string
  shouldShowTextCursor: boolean
}

export default function MarkdownRenderer(mainProps: MarkdownRendererProps) {
  const [lastLine, setLastLine] = React.useState(1)

  const plugin = React.useCallback(() => {
    const transformer = (ast: any) => {
      setLastLine(ast.position.end.line)
    }

    return transformer
  }, [mainProps.text])

  return (
    <div className={styles.markdownContainer}>
      <Markdown
        rehypePlugins={[plugin]}
        className={styles.markdownContainer}
        children={mainProps.text}
        components={{
          p: (props) => {
            const currentLine = props.node?.position?.end.line
            return (
              <p>
                {props.children}
                {currentLine === lastLine && mainProps.shouldShowTextCursor && (
                  <span className={styles.textCursor}>
                    <CaretSVG />
                  </span>
                )}
              </p>
            )
          },
          code: (props) => {
            const { children, className } = props
            const match = /language-(\w+)/.exec(className || '')
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
