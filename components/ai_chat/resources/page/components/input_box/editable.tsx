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

const assignContent = (el: HTMLElement, content: Content) => {
  const nodes = content.map(createNode)
  el.replaceChildren(...nodes)
}

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
  initialContent: Content
  onContentChange: (content: Content) => void

  onPaste?: React.ClipboardEventHandler
  placeholder?: string
}

export default React.forwardRef<HTMLDivElement, EditableProps>(function Editable({
  initialContent,
  onContentChange,
  placeholder,
  onPaste,
}: EditableProps, ref: React.ForwardedRef<HTMLElement>) {
  const onContentChangeRef = React.useRef(onContentChange)
  onContentChangeRef.current = onContentChange

  const refFunc = React.useCallback((el: HTMLElement | null) => {
    // Let the parent component know about the ref, if it needs to.
    if (typeof ref === 'function') {
      ref(el)
    } else if (ref) {
      ref.current = el
    }
    if (!el) return

    // Note: We only mount the content at the beginning of the conversation
    // because updating the nodes while typing will cause the cursor to jump.
    // Once the content has been assigned, the HTML is the source of truth.
    assignContent(el, initialContent)

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
      onContentChangeRef.current(parseContent(el))
    })

    mutationObserver.observe(el, {
      childList: true,
      characterData: true,
      // Note: Subtree is required here because we need to watch changes to the
      // child text nodes.
      subtree: true,
    })
  }, [])

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
