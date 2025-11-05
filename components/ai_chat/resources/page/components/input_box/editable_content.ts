// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Note: ContentNodes are stored in the html in the dataset of the node
// so all properties here need to be serializable as attributes.
export type ContentNode =
  | string
  | {
      type: 'skill'
      id: string
      text: string
    }

// Content of the component should be an array which can mix strings and block types.
export type Content = ContentNode[]

/**
 * Converts the content to a string. Once the mojom types are updated we
 * probably won't need to use this much.
 * @param content The content
 * @returns A stringified version of the content
 */
export const stringifyContent = (content: Content): string => {
  return content.map((c) => (typeof c === 'string' ? c : c.text)).join('')
}

/**
 * Assigns the content to the editable element.
 * @param el The editable element
 * @param content The content to assign
 */
export const assignContent = (el: HTMLElement, content: Content) => {
  const nodes = content.map(createDOMNodeRepresentation)
  el.replaceChildren(...nodes)
}

/**
 * Parses the content from the editable element in a Content array.
 * Note: Content is reconscructed from the dataset of childNodes, so if there is
 * non serializable content, it will be lost.
 * @param editable The editable element
 * @returns The parsed content
 */
export const createContentFromDOMNodes = (editable: HTMLElement): Content => {
  const content: Content = []
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

const createDOMNodeRepresentation = (node: ContentNode) => {
  if (typeof node === 'string') {
    return document.createTextNode(node)
  }

  // Ideally we'd use a leo-label here, but shadowRoot does not play nice with
  // contenteditable.
  if (node.type === 'skill') {
    const el = document.createElement('span')
    el.contentEditable = 'false'
    el.dataset.text = node.text
    el.dataset.type = node.type
    el.dataset.id = node.id
    el.textContent = node.text

    return el
  }

  throw new Error('Unknown content type: ' + JSON.stringify(node))
}

class EditorAPI {
  constructor(private readonly target: HTMLElement) {}

  private get canEdit() {
    let editingNode = document.getSelection()?.anchorNode
    while (editingNode) {
      if (editingNode === this.target) return true
      editingNode = editingNode.parentNode
    }
    return false
  }

  selectRange(range: Range) {
    if (!this.canEdit) return this

    const selection = window.getSelection()
    if (!selection) return

    selection.removeAllRanges()
    selection.addRange(range)

    return this
  }

  insertAtCursor(contentNode: ContentNode) {
    if (!this.canEdit) return this

    const node = createDOMNodeRepresentation(contentNode)
    const range = window.getSelection()!.getRangeAt(0)

    range.insertNode(node)

    // Make sure the node isn't selected by collapsing the range
    // to the end.
    range.collapse()

    return this
  }

  selectRangeToTriggerChar(triggerChar: string) {
    if (!this.canEdit) return this

    const selection = window.getSelection()!
    const range = selection.getRangeAt(0).cloneRange()

    const children: Node[] = Array.from(this.target.childNodes)
    let index = children.indexOf(range.startContainer)

    for (let i = index; i >= 0; i--) {
      const child = children[i]
      if (child.nodeType !== Node.TEXT_NODE) {
        continue
      }
      const text = child.textContent ?? ''
      const triggerIndex = text.indexOf(triggerChar)
      if (triggerIndex === -1) {
        continue
      }

      range.setStart(child, triggerIndex)
      break
    }

    selection.removeAllRanges()
    selection.addRange(range)

    return this
  }

  replaceSelectedRange(contentNode: ContentNode) {
    if (!this.canEdit) return this

    const selection = window.getSelection()!
    const range = selection.getRangeAt(0)

    const node = createDOMNodeRepresentation(contentNode)
    range.deleteContents()
    range.insertNode(node)
    range.collapse()

    // Move the cursor to the end of the replaced range.
    if (selection) {
      selection.removeAllRanges()
      selection.addRange(range)
    }

    return this
  }
}

export const makeEdit = (target: HTMLElement) => new EditorAPI(target)
