/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Markdown from 'react-markdown'
import remarkGfm from 'remark-gfm'
import remarkDirective from 'remark-directive'
import type { Root, Element as HastElement } from 'hast'
import Label from '@brave/leo/react/label'
import { visit } from 'unist-util-visit'

import styles from './style.module.scss'
import CaretSVG from '../svg/caret'
import {
  ALLOWED_DIRECTIVES,
  directiveComponents,
  remarkDirectives,
} from './remark_directives'
import { remarkColor, ColorChip } from './remark_color'
import {
  checkboxRenderer,
  createLiClickHandler,
  rehypeTaskCheckboxIndex,
} from './todo_list'

const CodeBlock = React.lazy(async () => ({
  default: (await import('../code_block')).default.Block,
}))
const CodeInline = React.lazy(async () => ({
  default: (await import('../code_block')).default.Inline,
}))

const allowedElements = [
  // Headings
  'h1',
  'h2',
  'h3',
  'h4',
  'h5',
  'h6',

  // Checkboxes
  'input',

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
  'a',

  // Tables
  'table',
  'thead',
  'tbody',
  'tr',
  'th',
  'td',

  // Directives
  ...ALLOWED_DIRECTIVES,

  // Color chips
  'colorchip',
]

interface CursorDecoratorProps {
  as: React.ElementType
  children: React.ReactNode
  isCursorVisible: boolean
  onClickCapture?: React.MouseEventHandler
}

function CursorDecorator(props: CursorDecoratorProps) {
  const Tag = props.as

  return (
    <Tag onClickCapture={props.onClickCapture}>
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
  // URLs sourced from the response's citations. Only links whose href matches
  // one of these render as numbered citation chips.
  allowedLinks?: string[]
}

export function RenderLink(props: RenderLinkProps) {
  const { a, allowedLinks } = props
  const { href, children } = a

  // Computed. All HTTPS links are allowed; other schemes (e.g. http) are not.
  const isLinkAllowed = href?.toLowerCase().startsWith('https://') ?? false

  const handleLinkClicked = React.useCallback(() => {
    if (href && isLinkAllowed) {
      window.open(href, '_blank', 'noopener noreferrer')
    }
  }, [href])

  if (!isLinkAllowed) {
    // Completely hide relative links.
    if (href?.startsWith('/')) {
      return null
    }
    return <span>{children}</span>
  }

  // Only links pointing at a citation source become numbered citation chips.
  // Other numeric-text links render as ordinary anchors.
  const isCitation =
    typeof children === 'string'
    && /^\d+$/.test(children)
    && (allowedLinks?.some((link) => href?.startsWith(link)) ?? false)

  if (isCitation) {
    return (
      <Label>
        <button
          className={styles.citation}
          onClick={handleLinkClicked}
        >
          {children}
        </button>
      </Label>
    )
  }

  return (
    <a
      // Pass the href so link previews continue to work.
      href={href}
      className={styles.conversationLink}
      target='_blank'
      rel='noopener noreferrer'
    >
      {children}
    </a>
  )
}

// Helper function to process content and convert <br> tags to line breaks
function processBrTags(children: React.ReactNode): React.ReactNode {
  return React.Children.map(children, (child) => {
    if (typeof child === 'string') {
      // Split by <br> tags and create line breaks
      const parts = child.split(/<br\s*\/?>/gi)
      if (parts.length === 1) {
        return child
      }
      return parts.map((part, index) => (
        <React.Fragment key={index}>
          {index !== 0 && <br />}
          {part}
        </React.Fragment>
      ))
    }
    return child
  })
}

// Helper function to extract text content from React nodes
function extractTextContent(node: React.ReactNode): string {
  if (typeof node === 'string') {
    return node
  }
  if (typeof node === 'number') {
    return String(node)
  }
  if (React.isValidElement(node)) {
    const { children } = node.props as { children?: React.ReactNode }
    if (children) {
      return React.Children.map(children, extractTextContent)?.join('') || ''
    }
  }
  return ''
}

function buildTableRenderer() {
  // For table header tracking
  const tableHeaders: (string | null)[] = []
  let columnIndex = 0

  return {
    table: (props: { children: React.ReactNode }) => {
      // Reset headers for each table
      tableHeaders.length = 0
      return (
        <div className={styles.tableWrapper}>
          <table className={styles.table}>{props.children}</table>
        </div>
      )
    },
    thead: (props: { children: React.ReactNode }) => (
      <thead className={styles.tableHead}>{props.children}</thead>
    ),
    tbody: (props: { children: React.ReactNode }) => {
      // Reset row index for each tbody
      columnIndex = 0
      return <tbody className={styles.tableBody}>{props.children}</tbody>
    },
    tr: (props: { children: React.ReactNode }) => {
      // Reset row index for each tr
      columnIndex = 0
      return <tr className={styles.tableRow}>{props.children}</tr>
    },
    th: (props: { children: React.ReactNode }) => {
      // Store header text (process content to handle <br> tags)
      const processedChildren = processBrTags(props.children)

      const text =
        React.Children.map(processedChildren, extractTextContent)?.join(' ')
        || null
      tableHeaders.push(text)
      return <th className={styles.tableHeader}>{processedChildren}</th>
    },
    td: (props: { children: React.ReactNode }) => {
      // Assign data-label from headers
      const label = tableHeaders[columnIndex]
      columnIndex++
      return (
        <td
          className={styles.tableCell}
          data-label={label}
        >
          {processBrTags(props.children)}
        </td>
      )
    },
  }
}

interface MarkdownRendererProps {
  text: string
  shouldShowTextCursor: boolean
  // Citation source URLs. Links pointing at these render as citation chips.
  allowedLinks?: string[]
  // Fires when the user toggles a GFM task-list checkbox. `index` is the
  // zero-based position of the checkbox among task-list checkboxes in
  // document order — matches findTaskCheckboxBracketOffsets() over the
  // source string the renderer was given. Omit to make checkboxes
  // non-interactive (e.g. while streaming).
  onToggleCheckbox?: (index: number, checked: boolean) => void
}

// Module-level constant so the array reference is stable across all renders.
const REMARK_PLUGINS = [
  remarkGfm,
  remarkDirective,
  remarkDirectives,
  remarkColor,
]

export default function MarkdownRenderer(mainProps: MarkdownRendererProps) {
  const lastElementRef = React.useRef<HastElement | undefined>(undefined)

  // Store changing props in refs so component functions can read the latest
  // values without being recreated when those props change.
  const allowedLinksRef = React.useRef(mainProps.allowedLinks)
  allowedLinksRef.current = mainProps.allowedLinks
  const onToggleCheckboxRef = React.useRef(mainProps.onToggleCheckbox)
  onToggleCheckboxRef.current = mainProps.onToggleCheckbox

  const plugin = React.useCallback(() => {
    const transformer = (tree: Root) => {
      const lastElLineEndsAt = tree.position?.end.line
      const lastElCharEndsAt = tree.position?.end.offset

      visit(tree, 'element', (el: HastElement) => {
        if (
          lastElLineEndsAt === el.position?.end.line
          && lastElCharEndsAt === el.position?.end.offset
        ) {
          lastElementRef.current = el
        }
      })
    }

    return transformer
  }, [])

  // Empty deps: all captured values are refs, so function references never
  // change. Stable references prevent react-markdown from seeing new component
  // types on re-renders, which would unmount/remount DOM nodes and re-fire
  // CSS animations.
  const components = React.useMemo(
    () => ({
      p: (props: React.ComponentProps<'p'> & { node?: HastElement }) => (
        <CursorDecorator
          as='p'
          children={props.children}
          isCursorVisible={props.node === lastElementRef.current}
        />
      ),
      li: (props: React.ComponentProps<'li'> & { node?: HastElement }) => (
        <CursorDecorator
          as='li'
          onClickCapture={createLiClickHandler((index, checked) =>
            onToggleCheckboxRef.current?.(index, checked),
          )}
          children={props.children}
          isCursorVisible={props.node === lastElementRef.current}
        />
      ),
      code: (props: React.ComponentProps<'code'>) => {
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
          allowedLinks={allowedLinksRef.current}
        />
      ),
      input: checkboxRenderer,
      colorchip: ColorChip,
      ...buildTableRenderer(),
      ...directiveComponents,
    }),
    [],
  )

  const rehypePlugins = React.useMemo(
    () =>
      mainProps.shouldShowTextCursor
        ? [rehypeTaskCheckboxIndex, plugin]
        : [rehypeTaskCheckboxIndex],
    [mainProps.shouldShowTextCursor, plugin],
  )

  // The per-block reveal animation only makes sense while content is
  // streaming in. Once the entry has finished generating, suppress it so
  // edits (e.g. checkbox toggles via ModifyConversation) don't refire the
  // animation across every block on re-parse.
  const containerClassName = mainProps.shouldShowTextCursor
    ? styles.markdownContainer
    : `${styles.markdownContainer} ${styles.noAnimation}`

  return (
    <div className={containerClassName}>
      <Markdown
        allowedElements={allowedElements}
        // The cursor-tracking plugin only runs when the text cursor is
        // allowed. rehypeTaskCheckboxIndex always runs so that clicking a
        // task-list checkbox can be mapped back to a position in the
        // source string.
        rehypePlugins={rehypePlugins}
        remarkPlugins={REMARK_PLUGINS}
        unwrapDisallowed={true}
        children={mainProps.text}
        components={components as any}
      />
    </div>
  )
}
