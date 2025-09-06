// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from "react";
import styles from './editable.module.scss'
import { createNode } from "./ranges";

// Note: ContentNodes are stored in the html in the dataset of the node
// so all properties here need to be serializable as attributes.
export type ContentNode = string | {
  type: 'associated-content',
  id: string,
  text: string
}
export type Content = ContentNode[]

/**
 * Converts the content to a string. Once the mojom types are updated we
 * probably won't need to use this much.
 * @param content The content
 * @returns A stringified version of the content
 */
export const stringifyContent = (content: Content): string => {
  return content.map(c => typeof c === 'string' ? c : c.text).join('')
}

/**
 * Assigns the content to the editable element.
 * @param el The editable element
 * @param content The content to assign
 */
const assignContent = (el: HTMLElement, content: Content) => {
  const nodes = content.map(createNode)
  el.replaceChildren(...nodes)
}

/**
 * Parses the content from the editable element in a Content array.
 * Note: Content is reconscructed from the dataset of childNodes, so if there is
 * non serializable content, it will be lost.
 * @param editable The editable element
 * @returns The parsed content
 */
const parseContent = (editable: HTMLElement): Content => {
  const content: ContentNode[] = []
  for (const child of editable.childNodes) {
    if (child.nodeType === Node.TEXT_NODE) {
      content.push(child.textContent ?? '')
    } else if (child.nodeType === Node.ELEMENT_NODE) {
      content.push({
        ...(child as HTMLElement).dataset,
      } as any)
    }
  }

  return content
}

export interface EditableProps {
  content: Content
  onContentChange: (content: Content) => void

  onPaste?: React.ClipboardEventHandler
  placeholder?: string
}

export default React.forwardRef<HTMLDivElement, EditableProps>(function Editable({
  content,
  onContentChange,
  placeholder,
  onPaste,
}: EditableProps, ref: React.ForwardedRef<HTMLElement>) {
  const onContentChangeRef = React.useRef(onContentChange)
  onContentChangeRef.current = onContentChange

  const elRef = React.useRef<HTMLElement | null>(null)
  const lastContentRef = React.useRef<Content | null>(null)

  const refFunc = React.useCallback((el: HTMLElement | null) => {
    elRef.current = el

    // Let the parent component know about the ref, if it needs to.
    if (typeof ref === 'function') {
      ref(el)
    } else if (ref) {
      ref.current = el
    }
    if (!el) return

    // Note: We use a MutationObserver to update the content when:
    // 1. The user types in the contenteditable element
    // 2. We programmatically replace the content
    // We could work around this, but it would be easy to miss updates, so this
    // is the simplest, safest solution.
    const mutationObserver = new MutationObserver(() => {
      // Note: We need to use `display: block` on the editable element to fix
      // some cursor positioning issues. However, this means the browser will
      // helpfully insert a <br> node at the end of the content, so we need to
      // trim it again.
      const lastNode = el.childNodes[el.childNodes.length - 1]
      if (lastNode &&'tagName' in lastNode && lastNode.tagName === 'BR') {
        el.removeChild(lastNode)
        return
      }

      // Note: We track the last content we parsed from the editable element so
      // we can tell if we need to update the content from what's passed in by
      // the parent component.
      const content = parseContent(el)
      lastContentRef.current = content
      onContentChangeRef.current(content)
    })

    mutationObserver.observe(el, {
      childList: true,
      characterData: true,
      // Note: Subtree is required here because we need to watch changes to the
      // child text nodes.
      subtree: true,
    })
  }, [])

  // If the content changes and it wasn't triggered by the user editing
  // the content then we should update it.
  React.useEffect(() => {
    if (lastContentRef.current === content || !elRef.current) return
    assignContent(elRef.current, content)
  }, [content])

  return <span
    ref={refFunc}
    data-editor
    autoFocus
    data-placeholder={placeholder}
    className={styles.editable}
    contentEditable='plaintext-only'
    onPaste={onPaste}
  />
})
