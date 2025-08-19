// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { ContentNode } from "./editable"

export const createNode = (node: ContentNode) => {
  if (typeof node === 'string') {
    return document.createTextNode(node)
  }

  const span = document.createElement('span')
  span.contentEditable = 'false'
  span.dataset.text = node.text
  span.dataset.type = node.type
  span.dataset.id = node.id
  span.textContent = `@${node.text}`
  return span
}

export const findParentEditable = (node: Node | null): HTMLElement | undefined => {
  if (node instanceof HTMLElement && node.dataset.editor) {
    return node
  }

  return node?.parentNode ? findParentEditable(node.parentNode) : undefined
}

export const selectRange = (range: Range) => {
  const selection = window.getSelection()
  if (!selection) return

  selection.removeAllRanges()
  selection.addRange(range)
}

export const insertAtCursor = (contentNode: ContentNode) => {
  const selection = window.getSelection()
  if (!selection) return
  if (!findParentEditable(selection.anchorNode)) return

  const node = createNode(contentNode)

  const range = selection.getRangeAt(0)
  range.insertNode(node)

  // Make sure the node isn't selected by collapsing the range
  // to the end.
  range.collapse()
}

export const getRangeToTriggerChar = (triggerChar: string) => {
  const selection = window.getSelection()
  if (!selection) return
  if (!findParentEditable(selection.anchorNode)) return

  const range = selection.getRangeAt(0).cloneRange()
  const editable = findParentEditable(range.startContainer)

  if (!editable) return

  const children: Node[] = Array.from(editable.childNodes)
  let index = children.indexOf(range.startContainer)

  for (let i = index; i >= 0; i--) {
    const child = children[i]
    if (child.nodeType !== Node.TEXT_NODE) {
      continue;
    }
    const text = child.textContent ?? ''
    const triggerIndex = text.indexOf(triggerChar)
    if (triggerIndex === -1) {
      continue
    }

    range.setStart(child, triggerIndex)
    break
  }

  return range
}

export const replaceRange = (range: Range, content: ContentNode) => {
  const node = createNode(content)

  range.deleteContents()
  range.insertNode(node)
  range.collapse()

  const selection = window.getSelection()
  if (!selection) return

  selection.removeAllRanges()
  selection.addRange(range)
}

export const clearInput = (editable?: HTMLElement) => {
  const selection = window.getSelection()
  if (!selection || !selection.rangeCount) return

  if (!editable) {
    editable = findParentEditable(selection.anchorNode)
      ?? document.querySelector('[data-editor]') as HTMLElement
    if (!editable) return
  }

  const range = selection.getRangeAt(0)
  range.selectNodeContents(editable)
  range.deleteContents()
}

export const setInputText = (text: string, editable?: HTMLElement, ) => {
  const selection = window.getSelection()
  if (!selection) return

  if (!editable) {
    editable = document.querySelector('[data-editor]') as HTMLElement
    if (!editable) return
  }

  const range = selection.getRangeAt(0)
  range.selectNodeContents(editable)
  range.insertNode(document.createTextNode(text))
  range.collapse()
}
