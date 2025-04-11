/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Markdown from 'react-markdown'
import type { Root, Element as HastElement } from 'hast'
import { Url } from 'gen/url/mojom/url.mojom.m.js'

import { visit } from 'unist-util-visit'

import styles from './style.module.scss'
import CaretSVG from '../svg/caret'
import {
  useUntrustedConversationContext //
} from '../../untrusted_conversation_context'

const removeReasoning = (text: string) => {
  return text.includes('<think>') ? text.split('</think>')[1] : text
}

const CodeBlock = React.lazy(async () => ({
  default: (await import('../code_block')).default.Block
}))
const CodeInline = React.lazy(async () => ({
  default: (await import('../code_block')).default.Inline
}))

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
  'hr',

  // Hyperlinks
  'a'
]

interface CursorDecoratorProps {
  as: React.ElementType
  children: React.ReactNode
  isCursorVisible: boolean
}

function CursorDecorator(props: CursorDecoratorProps) {
  const Tag = props.as

  return (
    <Tag>
      {props.children}
      {props.isCursorVisible && (
        <span className={styles.textCursor}>
          <CaretSVG />
        </span>
      )}
    </Tag>
  )
}

interface RenderLinkProps {
  a: React.ComponentProps<'a'>
  allowedLinks?: string[]
  disableLinkRestrictions?: boolean
}

export function RenderLink(props: RenderLinkProps) {
  const { a, allowedLinks, disableLinkRestrictions} = props
  const { href, children } = a

  // Context
  const context = useUntrustedConversationContext()

  // Computed
  const isHttps = href?.toLowerCase().startsWith('https://')
  const isLinkAllowed = isHttps && (disableLinkRestrictions ||
    (allowedLinks?.some((link) => href?.startsWith(link)) ?? false))

  const handleLinkClicked = React.useCallback(() => {
    if (href && isLinkAllowed) {
      const mojomUrl = new Url()
      mojomUrl.url = href
      context.parentUiFrame?.userRequestedOpenGeneratedUrl(mojomUrl)
    }
  }, [context, href])

  if (!isLinkAllowed) {
    return <span>{children}</span>
  }

  const isCitation = typeof children === 'string' && /^\d+$/.test(children)

  return (
    <a
      // While we preventDefault, we still need to pass the href
      // here so we can continue to show link previews.
      href={href}
      className={`${styles.conversationLink}${
        isCitation ? ` ${styles.citation}` : ''
      }`}
      onClick={(e) => {
        e.preventDefault()
        handleLinkClicked()
      }}
    >
      {children}
    </a>
  )
}

interface MarkdownRendererProps {
  text: string
  shouldShowTextCursor: boolean
  allowedLinks?: string[]
  disableLinkRestrictions?: boolean
}

export default function MarkdownRenderer(mainProps: MarkdownRendererProps) {
  const lastElementRef = React.useRef<HastElement | undefined>()

  const plugin = React.useCallback(() => {
    const transformer = (tree: Root) => {
      const lastElLineEndsAt = tree.position?.end.line
      const lastElCharEndsAt = tree.position?.end.offset

      visit(tree, 'element', (el: HastElement) => {
        if (
          lastElLineEndsAt === el.position?.end.line &&
          lastElCharEndsAt === el.position?.end.offset
        ) {
          lastElementRef.current = el
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
        children={removeReasoning(mainProps.text)}
        components={{
          p: (props) => (
            <CursorDecorator
              as='p'
              children={props.children}
              isCursorVisible={
                (props.node as HastElement) === lastElementRef.current
              }
            />
          ),
          li: (props) => (
            <CursorDecorator
              as='li'
              children={props.children}
              isCursorVisible={
                (props.node as HastElement) === lastElementRef.current
              }
            />
          ),
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
          },
          a: (props: any) => (
            <RenderLink
              a={props}
              allowedLinks={mainProps.allowedLinks}
              disableLinkRestrictions={mainProps.disableLinkRestrictions}
            />
          )
        }}
      />
    </div>
  )
}
