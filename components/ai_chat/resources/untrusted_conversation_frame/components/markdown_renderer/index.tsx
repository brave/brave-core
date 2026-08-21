/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Markdown from 'react-markdown'
import remarkGfm from 'remark-gfm'
import remarkDirective from 'remark-directive'
import remarkMath from 'remark-math'
import type { Root, Element as HastElement } from 'hast'
import type { PluggableList } from 'unified'
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
  IS_MATH_RENDERING_ENABLED,
  MATH_BLOCK_TAG,
  MATH_INLINE_TAG,
  MATH_REMARK_OPTIONS,
  remarkMathElements,
} from './remark_math'
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
// KaTeX and its stylesheet are ~300KB, so they load as their own chunk rather
// than sitting in the frame's initial bundle. This is only possible because the
// LaTeX is rendered by a component: a rehype plugin would have to run inside
// react-markdown's synchronous pipeline and could not await the import.
const MathBlock = React.lazy(async () => ({
  default: (await import('../math_block')).default.Block,
}))
const MathInline = React.lazy(async () => ({
  default: (await import('../math_block')).default.Inline,
}))

// Exported for tests: the renderer drops (and, with `unwrapDisallowed`,
// unwraps) any element not named here, so it is the effective allowlist for
// everything model output can produce.
export const allowedElements = [
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

  // Math. Omitted entirely when the feature is off so the kill switch also
  // closes the allowlist, rather than leaving tags nothing can produce.
  ...(IS_MATH_RENDERING_ENABLED ? [MATH_INLINE_TAG, MATH_BLOCK_TAG] : []),
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

// Returns true when `href` resolves to the same canonical URL as one of the
// citation sources. This compares URL identity rather than a string prefix so
// that a look-alike host (e.g. `https://brave.com.evil.example`) or a userinfo
// prefix (e.g. `https://brave.com@evil.example`) cannot masquerade as the cited
// `https://brave.com` and render as a trusted citation chip.
function isCitationUrl(
  href: string | undefined,
  allowedLinks: string[] | undefined,
): boolean {
  if (href === undefined || allowedLinks === undefined) {
    return false
  }
  let canonicalHref: string
  try {
    canonicalHref = new URL(href).href
  } catch {
    return false
  }
  return allowedLinks.some((link) => {
    try {
      return new URL(link).href === canonicalHref
    } catch {
      return false
    }
  })
}

export function RenderLink(props: RenderLinkProps) {
  const { a, allowedLinks } = props
  const { href, children } = a

  // Computed. All HTTPS links are allowed; other schemes (e.g. http) are not.
  const isLinkAllowed = href?.toLowerCase().startsWith('https://') ?? false

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
    && isCitationUrl(href, allowedLinks)

  if (isCitation) {
    // Render as an anchor (not a button) so hovering discloses the destination
    // via the browser status bubble, matching a normal tab.
    return (
      <Label>
        <a
          className={styles.citation}
          href={href}
          target='_blank'
          rel='noopener noreferrer'
        >
          {children}
        </a>
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
const REMARK_PLUGINS: PluggableList = [
  remarkGfm,
  remarkDirective,
  remarkDirectives,
  remarkColor,
  // remarkMath only registers parser extensions, so remarkMathElements always
  // sees the math nodes it produces regardless of their relative order here.
  ...(IS_MATH_RENDERING_ENABLED
    ? ([[remarkMath, MATH_REMARK_OPTIONS], remarkMathElements] as PluggableList)
    : []),
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
      // The element's only child is the LaTeX source (see remarkMathElements).
      // While the KaTeX chunk loads, fall back to that source rather than a
      // placeholder so the expression stays readable.
      [MATH_INLINE_TAG]: (props: { children?: React.ReactNode }) => {
        const tex = String(props.children ?? '')
        return (
          <React.Suspense fallback={tex}>
            <MathInline tex={tex} />
          </React.Suspense>
        )
      },
      [MATH_BLOCK_TAG]: (props: { children?: React.ReactNode }) => {
        const tex = String(props.children ?? '')
        return (
          <React.Suspense fallback={tex}>
            <MathBlock tex={tex} />
          </React.Suspense>
        )
      },
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
