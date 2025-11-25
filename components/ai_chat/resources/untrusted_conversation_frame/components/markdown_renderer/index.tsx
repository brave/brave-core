/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * Markdown Renderer with Streaming Text Fade-in Animation
 *
 * This component renders markdown content with support for streaming text
 * animations.
 * When text is streamed in (during AI response generation), new text chunks are
 * automatically wrapped in fade-in spans to create a smooth appearance effect.
 *
 * Key features:
 * - Tracks text changes per element using a ref stored in the parent component
 * - Wraps new text nodes in spans with fade-in animation
 * - Excludes links and code blocks from fade-in to preserve functionality
 * - Handles strict HTML content models (tables, lists) correctly
 */

import * as React from 'react'
import Markdown from 'react-markdown'
import remarkGfm from 'remark-gfm'
import type { Root, Element as HastElement } from 'hast'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import Label from '@brave/leo/react/label'
import classnames from '$web-common/classnames'
import styles from './style.module.scss'
import {
  useUntrustedConversationContext, //
} from '../../untrusted_conversation_context'

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

  // Inline elements
  'span',

  // Tables
  'table',
  'thead',
  'tbody',
  'tr',
  'th',
  'td',
]

interface RenderLinkProps {
  a: React.ComponentProps<'a'>
  allowedLinks?: string[]
  disableLinkRestrictions?: boolean
}

export function RenderLink(props: RenderLinkProps) {
  const { a, allowedLinks, disableLinkRestrictions } = props
  const { href, children } = a

  // Context
  const context = useUntrustedConversationContext()

  // Computed
  const isHttps = href?.toLowerCase().startsWith('https://')
  const isLinkAllowed =
    isHttps
    && (disableLinkRestrictions
      || (allowedLinks?.some((link) => href?.startsWith(link)) ?? false))

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

  if (isCitation) {
    return (
      <Label>
        <a
          // While we preventDefault, we still need to pass the href
          // here so we can continue to show link previews.
          href={href}
          className={styles.citation}
          onClick={(e) => {
            e.preventDefault()
            handleLinkClicked()
          }}
        >
          {children}
        </a>
      </Label>
    )
  }

  return (
    <a
      // While we preventDefault, we still need to pass the href
      // here so we can continue to show link previews.
      href={href}
      className={styles.conversationLink}
      onClick={(e) => {
        e.preventDefault()
        handleLinkClicked()
      }}
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
    if (node.props.children) {
      return (
        React.Children.map(node.props.children, extractTextContent)?.join('')
        || ''
      )
    }
  }
  return ''
}

function buildTableRenderer() {
  // For table header tracking
  const tableHeaders: string[] = []
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
        || ''
      if (text) tableHeaders.push(text)
      return <th className={styles.tableHeader}>{processedChildren}</th>
    },
    td: (props: { children: React.ReactNode }) => {
      // Assign data-label from headers
      const label = tableHeaders[columnIndex] || ''
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
  isEntryInProgress: boolean
  allowedLinks?: string[]
  disableLinkRestrictions?: boolean
  elementTextStateRef?: React.MutableRefObject<Map<number, string>>
}

/**
 * Extracts all text content from a Hast (HTML AST) node recursively.
 * Used for tracking text length changes in elements for fade-in animation.
 */
function extractTextFromNode(node: any): string {
  if (node.type === 'text') {
    return node.value || ''
  }
  if (node.type === 'element' && node.children) {
    return node.children.map(extractTextFromNode).join('')
  }
  return ''
}

// Elements that have strict content models where spans aren't allowed as
// direct children
const STRICT_CONTENT_ELEMENTS = new Set([
  'table',
  'thead',
  'tbody',
  'tfoot',
  'tr',
  'colgroup',
  'ul',
  'ol',
])

// Helper to check if we can safely wrap text in a span in this context
function canWrapInSpan(parentTagName: string | undefined): boolean {
  if (!parentTagName) return true
  // Don't wrap if parent has strict content model
  return !STRICT_CONTENT_ELEMENTS.has(parentTagName.toLowerCase())
}

/**
 * Processes text nodes in the AST and wraps new text chunks in fade-in spans.
 * Tracks character position to identify which portions of text are newly
 * streamed. Excludes links and code blocks from fade-in wrapping to preserve
 * their functionality.
 */
function processTextNodesForElement(
  node: any,
  previousTextLength: number,
  currentTextLength: number,
  currentCharIndex: { value: number },
  parentTagName?: string,
  isInsideLink?: boolean,
  isInsideCode?: boolean,
): any {
  if (node.type === 'text') {
    const text = node.value || ''
    const textStart = currentCharIndex.value
    const textEnd = textStart + text.length

    // Don't wrap text inside links (including citations) or code blocks in
    // fade-in spans
    // If this text node contains characters beyond the previous length, it
    // has new content
    if (
      textEnd > previousTextLength
      && canWrapInSpan(parentTagName)
      && !isInsideLink
      && !isInsideCode
    ) {
      if (textStart < previousTextLength) {
        // Part of this text is old, part is new - split it
        const splitPoint = previousTextLength - textStart
        const oldText = text.substring(0, splitPoint)
        const newText = text.substring(splitPoint)

        currentCharIndex.value = textEnd

        return {
          type: 'element',
          tagName: 'span',
          properties: {},
          children: [
            { type: 'text', value: oldText },
            {
              type: 'element',
              tagName: 'span',
              properties: { className: 'fade-in-new-text' },
              children: [{ type: 'text', value: newText }],
            },
          ],
        }
      } else {
        // All of this text is new - wrap it entirely
        currentCharIndex.value = textEnd

        return {
          type: 'element',
          tagName: 'span',
          properties: { className: 'fade-in-new-text' },
          children: [{ type: 'text', value: text }],
        }
      }
    }

    // Text is not new or can't be wrapped, return as-is
    currentCharIndex.value = textEnd
    return node
  }

  if (node.type === 'element' && node.children) {
    const currentTagName = node.tagName || parentTagName
    const tagNameLower = currentTagName?.toLowerCase()
    // Check if this is a link or code element
    const isLink = tagNameLower === 'a'
    const isCode = tagNameLower === 'code' || tagNameLower === 'pre'
    // Process children recursively, maintaining character position
    const newChildren: any[] = []
    for (const child of node.children) {
      const processed = processTextNodesForElement(
        child,
        previousTextLength,
        currentTextLength,
        currentCharIndex,
        currentTagName,
        isInsideLink || isLink,
        isInsideCode || isCode,
      )
      newChildren.push(processed)
    }

    return {
      ...node,
      children: newChildren,
    }
  }

  // For other node types (like raw HTML, etc.), extract text to update
  // character index
  if (node.type !== 'text' && node.type !== 'element') {
    const nodeText = extractTextFromNode(node)
    currentCharIndex.value += nodeText.length
  }

  return node
}

/**
 * MarkdownRenderer component that renders markdown with streaming text
 * fade-in animations. Tracks text changes per element and wraps new text
 * chunks in fade-in spans. State is stored in parent component ref to persist
 * across re-renders during streaming.
 */
function MarkdownRenderer(mainProps: MarkdownRendererProps) {
  // Use the ref from parent if provided, otherwise create a local one
  // This allows the parent to persist state across MarkdownRenderer re-renders
  // which is crucial for tracking text changes during streaming
  const localElementTextStateRef = React.useRef<Map<number, string>>(new Map())
  const elementTextStateRef =
    mainProps.elementTextStateRef || localElementTextStateRef

  // Rehype plugin that processes the AST to add fade-in animations to new text
  const plugin = React.useCallback(() => {
    const transformer = (tree: Root) => {
      const treeNodes = tree.children

      // Collect only element nodes (filter out text nodes, comments, etc.)
      // We track text at the element level, not individual text nodes
      const elementNodes: HastElement[] = []
      for (const node of treeNodes) {
        if (node.type === 'element') {
          elementNodes.push(node)
        }
      }

      // Process each element to track text changes and wrap new text in
      // fade-in spans
      for (
        let elementIndex = 0;
        elementIndex < elementNodes.length;
        elementIndex++
      ) {
        const node = elementNodes[elementIndex]
        const currentText = extractTextFromNode(node)
        const previousTextLength =
          elementTextStateRef.current.get(elementIndex)?.length || 0
        const currentTextLength = currentText.length

        // If there's new text, process the node to wrap new chunks
        if (currentTextLength > previousTextLength) {
          // Update the stored text for this element
          elementTextStateRef.current.set(elementIndex, currentText)

          // Process the node to wrap new text in fade-in spans
          // Use a mutable object to track character position across recursive
          // calls
          const charIndex = { value: 0 }
          const nodeTagName = node.tagName
          const processedNode = processTextNodesForElement(
            node,
            previousTextLength,
            currentTextLength,
            charIndex,
            nodeTagName,
          )

          // Replace the node in the tree
          const nodeIndex = tree.children.indexOf(node)
          if (nodeIndex !== -1) {
            tree.children[nodeIndex] = processedNode
          }
        } else if (currentTextLength === 0 && previousTextLength > 0) {
          // Element was reset (content cleared), remove from tracking
          elementTextStateRef.current.delete(elementIndex)
        } else {
          // Text length unchanged or decreased - update stored text to reflect
          // current state.
          // This handles cases where content might have changed without
          // length change.
          elementTextStateRef.current.set(elementIndex, currentText)
        }
      }
    }

    return transformer
  }, [elementTextStateRef])

  return (
    <div
      className={classnames({
        [styles.markdownContainer]: true,
        [styles.isStreaming]: mainProps.isEntryInProgress,
      })}
    >
      <Markdown
        allowedElements={allowedElements}
        // We only read the total lines value from AST
        // if the component is allowed to show the text cursor.
        rehypePlugins={[plugin]}
        remarkPlugins={[remarkGfm]}
        unwrapDisallowed={true}
        children={mainProps.text}
        components={{
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
          ),
          span: (props: any) => {
            // Handle spans with fade-in class for new text
            // (created by rehype plugin)
            // The plugin wraps new text in spans with 'fade-in-new-text'
            // class
            const className = props.className
            let hasFadeIn = false
            if (className) {
              if (typeof className === 'string') {
                hasFadeIn = className.includes('fade-in-new-text')
              } else if (Array.isArray(className)) {
                hasFadeIn = className.includes('fade-in-new-text')
              } else {
                hasFadeIn = className === 'fade-in-new-text'
              }
            }
            // Apply fade-in animation style if this span contains new text
            const combinedClassName = hasFadeIn
              ? `${props.className || ''} ${styles.fadeIn}`.trim()
              : props.className
            return <span className={combinedClassName}>{props.children}</span>
          },
          ...buildTableRenderer(),
        }}
      />
    </div>
  )
}

/**
 * Memoized MarkdownRenderer to prevent unnecessary re-renders.
 * Only re-renders when text content, streaming state, or link configuration
 * changes.
 */
export default React.memo(MarkdownRenderer, (prevProps, nextProps) => {
  // Returns true if props are equal (skip re-render), false if they differ
  // (re-render)
  return (
    prevProps.text === nextProps.text
    && prevProps.isEntryInProgress === nextProps.isEntryInProgress
    && prevProps.disableLinkRestrictions === nextProps.disableLinkRestrictions
    && JSON.stringify(prevProps.allowedLinks)
      === JSON.stringify(nextProps.allowedLinks)
  )
})
