// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Notify the background script as soon as the content script has loaded.
// chrome.tabs.insertCSS may sometimes fail to inject CSS in a newly navigated
// page when using the chrome.webNavigation API.
// See: https://bugs.chromium.org/p/chromium/issues/detail?id=331654#c15
// The RenderView should always be ready when the content script begins, so
// this message is used to trigger CSS insertion instead.

let pickerFrame: HTMLIFrameElement | null

// When the picker is activated, it eats all pointer events and takes up the
// entire screen. All calls to document.elementFromPoint(..) will return the
// frame. We disable pointer events for the duration of the query to get
// around this.
const elementFromFrameCoords = (x: number, y: number): Element | null => {
  if (!pickerFrame) { return null }
  pickerFrame.style.setProperty('pointer-events', 'none', 'important')
  const elem = document.elementFromPoint(x, y)
  pickerFrame.style.setProperty('pointer-events', 'auto', 'important')
  return elem
}

enum SpecificityFlags {
  Id = (1 << 0),
  Hierarchy = (1 << 1),
  Attributes = (1 << 2),
  Class = (1 << 3),
  NthOfType = (1 << 4)
}

const mostSpecificMask = 0b11111

enum Selector {
  Id,
  Class,
  Attributes,
  NthOfType
}

interface Rule {
  type: Selector
  value: any
}

class ElementSelectorBuilder {
  public hasId: boolean
  private readonly elem: Element
  private readonly rules: Rule[]
  private tag: string

  constructor (elem: Element) {
    this.rules = []
    this.tag = ''
    this.elem = elem
    this.hasId = false
  }

  addRule (rule: Rule): void {
    if (rule.type < Selector.Id || rule.type > Selector.NthOfType) {
      console.log(`Unexpected selector: ${rule.type}`)
      return
    }
    if (Array.isArray(rule.value) && rule.value.length === 0) {
      return
    }
    if (rule.type === Selector.Id) { this.hasId = true }
    this.rules.push(rule)
  }

  addTag (tag: string): void {
    this.tag = tag
  }

  size (): number {
    return this.rules.length
  }

  toString (mask: number = mostSpecificMask): string {
    let selector = this.tag + ''
    for (const rule of this.rules) {
      if (!(mask & SpecificityFlags.Id) && rule.type === Selector.Id) {
        continue
      }
      if (!(mask & SpecificityFlags.Class) && rule.type === Selector.Class) {
        continue
      }
      if (!(mask & SpecificityFlags.Attributes) && rule.type === Selector.Attributes) {
        continue
      }
      if (!(mask & SpecificityFlags.NthOfType) && rule.type === Selector.NthOfType) {
        continue
      }
      if (this.hasId && (mask & SpecificityFlags.Id) && rule.type === Selector.Class) {
        continue
      }

      switch (rule.type) {
        case Selector.Id: {
          selector += '#' + rule.value
          break
        }
        case Selector.Class: {
          selector += '.' + rule.value.join('.')
          break
        }
        case Selector.Attributes: {
          for (const attribute of rule.value) {
            const sourceAttr = this.elem.getAttribute(attribute.attr)
            let op = '*='
            if (attribute.attr === sourceAttr) {
              op = '='
            } else if (attribute.attr.startsWith(sourceAttr)) {
              op = '^='
            }
            selector += `[${attribute.attr}${op}"${attribute.value}"]`
          }
          break
        }
        case Selector.NthOfType: {
          // NOTE: this selector is not valid without calling addTag(..)
          selector += `:nth-of-type(${rule.value})`
          break
        }
        default: { /* Unreachable */ }
      }
    }
    return selector
  }
}

// We search for a CSS selector for the target element. We want the most specific identifiers.
const cssSelectorFromElement = (elem: Element): ElementSelectorBuilder => {
  const builder = new ElementSelectorBuilder(elem)

  // ID
  if (elem.id.length > 0) {
    builder.addRule({
      type: Selector.Id,
      value: CSS.escape(elem.id)
    })
  }

  // Class names
  if (elem.classList.length > 0) {
    builder.addRule({
      type: Selector.Class,
      value: Array.from(elem.classList).map((c: string) => CSS.escape(c))
    })
  }

  const tag = CSS.escape(elem.localName)

  // Attributes. Only try these if we have no matches.
  if (builder.size() === 0) {
    const attributes = []
    switch (tag) {
      case 'a': {
        // Get URL, removing query parameters and hash
        const url = elem.getAttribute('href')?.trim().split(/[?#]/)[0]
        if (url !== undefined && url.length > 0) {
          attributes.push({
            attr: 'href',
            value: url
          })
        }
        break
      }
      case 'iframe': {
        const url = elem.getAttribute('src')?.trim()
        if (url !== undefined && url.length > 0) {
          attributes.push({
            attr: 'src',
            value: url.slice(0, 256)
          })
        }
        break
      }
      case 'img': {
        let data = elem.getAttribute('src')?.trim()
        if (data !== undefined && data.length > 0) {
          // https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/Data_URIs
          if (data.startsWith('data:')) {
            data = data.split(',')[1].slice(0, 256)
          }
        }
        if (data === undefined || data.length === 0) {
          let alttext = elem.getAttribute('alt')?.trim()
          if (alttext !== undefined && alttext.length > 0) {
            attributes.push({
              attr: 'alt',
              value: alttext
            })
          }
        } else {
          attributes.push({
            attr: 'src',
            value: data
          })
        }
        break
      }
      default: { break }
    }
    if (attributes.length > 0) {
      builder.addRule({
        type: Selector.Attributes,
        value: attributes
      })
    }
  }

  const querySelectorNoExcept = (node: Element | null, selector: string): Element[] => {
    if (node !== null) {
      try {
        let r = node.querySelectorAll(selector)
        return Array.from(r)
      } catch (e) { /* Deliberately left empty */ }
    }
    return []
  }

  if (builder.size() === 0 || querySelectorNoExcept(elem.parentElement, builder.toString()).length > 1) {
    builder.addTag(tag)
    if (querySelectorNoExcept(elem.parentElement, builder.toString()).length > 1) {
      let index = 1
      let sibling: Element | null = elem.previousElementSibling
      while (sibling !== null) {
        if (sibling.localName === tag) { index++ }
        sibling = sibling.previousElementSibling
      }
      builder.addRule({
        type: Selector.NthOfType,
        value: index
      })
    }
  }
  return builder
}

const elementPickerOnKeydown = (event: KeyboardEvent): void => {
  if (event.key === 'Escape') {
    event.stopPropagation()
    event.preventDefault()
    quitElementPicker()
  }
}

const elementPickerViewportChanged = () => {
  recalculateAndSendTargets(targetedElems)
}

const quitElementPicker = () => {
  if (pickerFrame !== null) {
    document.documentElement.removeChild(pickerFrame)
  }

  // Tear down element picker listeners
  document.removeEventListener('keydown', elementPickerOnKeydown, true)
  document.removeEventListener('resize', elementPickerViewportChanged)
  document.removeEventListener('scroll', elementPickerViewportChanged)
}

const launchElementPicker = () => {
  pickerFrame = document.createElement('iframe')
  // "src" is a web accessible resource since the URI is chrome-extension://.
  // This ensures a malicious page cannot modify the iframe contents.
  pickerFrame.src = chrome.runtime.getURL('elementPicker.html')
  const pickerCSSStyle: string = [
    'background: transparent',
    'border: 0',
    'border-radius: 0',
    'box-shadow: none',
    'color-scheme: light dark',
    'display: block',
    'height: 100%',
    'left: 0',
    'margin: 0',
    'max-height: none',
    'max-width: none',
    'opacity: 1',
    'outline: 0',
    'padding: 0',
    'pointer-events: auto',
    'position: fixed',
    'top: 0',
    'visibility: visible',
    'width: 100%',
    'z-index: 2147483647',
    ''
  ].join(' !important;')
  // TODO(keur): To harden this we can use chrome.tabs.insertCSS(). Not a
  // big priority since the page can always remove the iframe if it wants.
  pickerFrame.style.cssText = pickerCSSStyle

  // We don't append to the body because we are setting the frame's
  // width and height to be 100%. Prevents the picker from only being
  // able to hover the iframe.
  document.documentElement.appendChild(pickerFrame)

  // Setup listeners to assist element picker
  document.addEventListener('keydown', elementPickerOnKeydown, true)
  document.addEventListener('resize', elementPickerViewportChanged)
  document.addEventListener('scroll', elementPickerViewportChanged)
}

interface TargetRect {
  x: number
  y: number
  width: number
  height: number
}

const targetRectFromElement = (elem: Element): TargetRect => {
  const rect = elem.getBoundingClientRect()
  return {
    x: rect.left,
    y: rect.top,
    width: rect.right - rect.left,
    height: rect.bottom - rect.top
  }
}

let lastHoveredElem: HTMLElement | null = null
let targetedElems: Element[] = []

const recalculateAndSendTargets = (elems: Element[]) => {
  targetedElems = elems
  const coords: TargetRect[] = elems.map((e: Element) => targetRectFromElement(e))
  chrome.runtime.sendMessage({ type: 'highlightElements', coords: coords })
}

const onTargetSelected = (selected: Element | null, index: number): string => {
  if (lastHoveredElem === null) { return '' }

  let elem: Element | null = selected
  const selectorBuilders = []
  const specificityMasks = [
    0b01101, // No DOM hierarchy, no nth-of-type
    0b11101, // No DOM hierarchy
    0b01011, // No nth-of-type, no attributes
    0b10011, // No attributes, no class names
    0b11111 // All selector rules (default)
  ]
  const mask: number = specificityMasks[index]

  if (mask & SpecificityFlags.Hierarchy) {
    while (elem !== null && elem !== document.body) {
      selectorBuilders.push(cssSelectorFromElement(elem))
      elem = elem.parentElement
    }
  } else {
    selectorBuilders.push(cssSelectorFromElement(selected!))
  }
  // TODO: insert the body if using nth-type-of

  let i = 0
  for (; i < selectorBuilders.length; i++) {
    const b = selectorBuilders[i]
    if ((mask & SpecificityFlags.Id) && b.hasId ||
        document.querySelectorAll(b.toString(mask)).length === 1) {
      break
    }
  }
  const selector = selectorBuilders
      .slice(0, i + 1)
      .reverse()
      .map((b) => b.toString(mask))
      .join(' > ')
  return selector
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'quitElementPicker':
      quitElementPicker()
      break
    case 'elementPickerHoverCoordsChanged': {
      const { coords } = msg
      const elem = elementFromFrameCoords(coords.x, coords.y)
      if (elem !== null && (elem instanceof HTMLElement) && elem !== lastHoveredElem) {
        recalculateAndSendTargets([elem])
        lastHoveredElem = elem
      }
      break
    }
    case 'elementPickerUserSelectedTarget': {
      const { specificity } = msg
      if (lastHoveredElem !== null && (lastHoveredElem instanceof HTMLElement)) {
        const selector = onTargetSelected(lastHoveredElem, specificity)
        recalculateAndSendTargets(Array.from(document.querySelectorAll(selector)))
        sendResponse({
          isValid: selector !== '',
          selector: selector.trim()
        })
      }
      break
    }
    case 'elementPickerUserModifiedRule': {
      const selector = msg.selector
      if (selector.length > 0) {
        recalculateAndSendTargets(Array.from(document.querySelectorAll(selector)))
      }
      break
    }
    case 'elementPickerUserCreatedRule': {
      // Append the hostname and forward to the background script
      chrome.runtime.sendMessage({
        type: 'cosmeticFilterCreate',
        selector: msg.selector
      })
      quitElementPicker()
      break
    }
  }
})

launchElementPicker()
