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
  | {
      type: 'action'
      id: string
      text: string
    }
  | {
      type: 'attachment'
      id: string
      text: string
      url: string
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
  return content
    .map((c) => {
      if (typeof c === 'string') return c
      if (c.type === 'skill') return c.text
      if (c.type === 'action') return c.text
      if (c.type === 'attachment') return `[mention(${c.text})]`
      throw new Error('Unknown content type: ' + JSON.stringify(c))
    })
    .join('')
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
 * Elements which WebKit/Blink may wrap lines in when editing or pasting
 * multi-line text into the editable (e.g. `<div>a</div><div>b</div>`).
 */
const BLOCK_TAGS = new Set(['DIV', 'P', 'LI'])

const isBlockElement = (node: Node | null): node is HTMLElement =>
  node?.nodeType === Node.ELEMENT_NODE
  && BLOCK_TAGS.has((node as HTMLElement).tagName)

/**
 * A <br> which is the last child of a block (or the editable itself) only
 * exists to give an empty line height, and doesn't represent a newline itself.
 */
const isPlaceholderBreak = (br: Node, editable: HTMLElement) => {
  const parent = br.parentNode
  if (parent !== editable && !isBlockElement(parent)) return false

  let next = br.nextSibling
  while (next?.nodeType === Node.TEXT_NODE && !next.textContent) {
    next = next.nextSibling
  }
  return !next
}

/**
 * Parses the content from the editable element in a Content array.
 * Note: Content is reconscructed from the dataset of content nodes, so if there
 * is non serializable content, it will be lost. Line breaks (<br>) and block
 * elements are converted to newlines.
 * @param editable The editable element
 * @returns The parsed content
 */
export const createContentFromDOMNodes = (editable: HTMLElement): Content => {
  const content: Content = []
  // Whether the last thing emitted was the newline closing a block element.
  let lastWasBlockEnd = false

  const endsWithNewline = () => {
    const last = content.findLast((c) => typeof c !== 'string' || c !== '')
    return typeof last === 'string' && last.endsWith('\n')
  }

  const walk = (parent: Node) => {
    for (const child of parent.childNodes) {
      if (child.nodeType === Node.TEXT_NODE) {
        const text = child.textContent ?? ''
        content.push(text)
        if (text) lastWasBlockEnd = false
        continue
      }

      if (child.nodeType !== Node.ELEMENT_NODE) continue
      const el = child as HTMLElement

      if (el.dataset.type) {
        content.push({ ...el.dataset } as any)
        lastWasBlockEnd = false
        continue
      }

      if (el.tagName === 'BR') {
        if (!isPlaceholderBreak(el, editable)) {
          content.push('\n')
          lastWasBlockEnd = false
        }
        continue
      }

      if (!isBlockElement(el)) {
        walk(el)
        continue
      }

      // Blocks always start on a new line.
      if (content.length && !endsWithNewline()) {
        content.push('\n')
      }
      lastWasBlockEnd = false
      walk(el)
      // Nested blocks share a single closing newline.
      if (!lastWasBlockEnd) {
        content.push('\n')
        lastWasBlockEnd = true
      }
    }
  }

  walk(editable)

  // The final block's closing newline doesn't start a new line.
  if (lastWasBlockEnd) {
    const index = content.findLastIndex((c) => c === '\n')
    content.splice(index, 1)
  }

  return content.filter((c) => typeof c === 'string' || 'type' in c)
}

const createDOMNodeRepresentation = (node: ContentNode) => {
  if (typeof node === 'string') {
    return document.createTextNode(node)
  }

  // Ideally we'd use a leo-label here, but shadowRoot does not play nice with
  // contenteditable.
  if (
    node.type === 'skill'
    || node.type === 'action'
    || node.type === 'attachment'
  ) {
    const el = document.createElement('span')
    el.contentEditable = 'false'
    el.dataset.text = node.text
    el.dataset.type = node.type
    el.dataset.id = node.id
    el.textContent = node.text

    if (node.type === 'attachment') {
      el.dataset.url = node.url
      el.setAttribute('title', node.url)
    }

    return el
  }

  throw new Error('Unknown content type: ' + JSON.stringify(node))
}

function maybeRestoreSelection(el: HTMLElement) {
  // If the element is active there's nothing to do here.
  if (document.activeElement === el) return

  if ('lastSelection' in el) {
    const selection = window.getSelection()
    if (!selection) return

    selection.removeAllRanges()
    selection.addRange(el.lastSelection as Range)
  }
}

class EditorAPI {
  constructor(private readonly target: HTMLElement) {}

  private get canEdit() {
    maybeRestoreSelection(this.target)

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

  ifHasSelection() {
    const selection = window.getSelection()
    return !selection?.isCollapsed ? this : undefined
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
