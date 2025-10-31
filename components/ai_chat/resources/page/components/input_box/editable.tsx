// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styles from './editable.module.scss'
import {
  assignContent,
  Content,
  createContentFromDOMNodes,
} from './editable_content'

export interface EditableProps {
  content: Content
  onContentChange: (content: Content) => void

  onPaste?: React.ClipboardEventHandler
  placeholder?: string
}

export default React.forwardRef<HTMLDivElement, EditableProps>(
  function Editable(
    { content, onContentChange, placeholder, onPaste }: EditableProps,
    ref: React.ForwardedRef<HTMLElement>,
  ) {
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
        if (lastNode && 'tagName' in lastNode && lastNode.tagName === 'BR') {
          el.removeChild(lastNode)
          return
        }

        // Note: We track the last content we parsed from the editable element so
        // we can tell if we need to update the content from what's passed in by
        // the parent component.
        const content = createContentFromDOMNodes(el)
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

    return (
      <span
        ref={refFunc}
        data-editor
        autoFocus
        data-placeholder={placeholder}
        className={styles.editable}
        contentEditable='plaintext-only'
        onPaste={onPaste}
      />
    )
  },
)
