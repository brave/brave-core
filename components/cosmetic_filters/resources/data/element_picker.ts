// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
const NSSVG = 'http://www.w3.org/2000/svg'

let pickerDiv: HTMLDivElement | null
let shadowRoot: ShadowRoot | null

const api = {
  cosmeticFilterCreate: (selector: string) => {
    cf_worker.addSiteCosmeticFilter(selector)

    const styleId = 'brave-content-picker-style'
    let style = document.getElementById(styleId)
    if (!style) {
      style = document.createElement('style')
      style.id = styleId
      document.head.appendChild(style)
    }
    style.innerText += `${selector} {display: none !important;}`
  },
  cosmeticFilterManage: () => {
    cf_worker.manageCustomFilters()
  },
}

// When the picker is activated, it eats all pointer events and takes up the
// entire screen. All calls to document.elementFromPoint(..) will return the
// frame. We disable pointer events for the duration of the query to get
// around this.
const elementFromFrameCoords = (x: number, y: number): Element | null => {
  if (!pickerDiv) {
    return null
  }
  pickerDiv.style.setProperty('pointer-events', 'none', 'important')
  const elem = document.elementFromPoint(x, y)
  pickerDiv.style.setProperty('pointer-events', 'auto', 'important')
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
      if (
        !(mask & SpecificityFlags.Attributes) &&
        rule.type === Selector.Attributes
      ) {
        continue
      }
      if (
        !(mask & SpecificityFlags.NthOfType) &&
        rule.type === Selector.NthOfType
      ) {
        continue
      }
      if (
        this.hasId &&
        mask & SpecificityFlags.Id &&
        rule.type === Selector.Class
      ) {
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

// We search for a CSS selector for the target element. We want the most
// specific identifiers.
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
          const alttext = elem.getAttribute('alt')?.trim()
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

  const querySelectorNoExcept = (
    node: Element | null,
    selector: string
  ): Element[] => {
    if (node !== null) {
      try {
        const r = node.querySelectorAll(selector)
        return Array.from(r)
      } catch { /* Deliberately left empty */ }
    }
    return []
  }

  if (
    builder.size() === 0 ||
    querySelectorNoExcept(elem.parentElement, builder.toString()).length > 1
  ) {
    builder.addTag(tag)
    if (
      querySelectorNoExcept(elem.parentElement, builder.toString()).length > 1
    ) {
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
  if (pickerDiv !== null) {
    document.documentElement.removeChild(pickerDiv)
  }

  // Tear down element picker listeners
  document.removeEventListener('keydown', elementPickerOnKeydown, true)
  document.removeEventListener('resize', elementPickerViewportChanged)
  document.removeEventListener('scroll', elementPickerViewportChanged)
}

const attachElementPicker = () => {
  // "src" is a web accessible resource since the URI is chrome-extension://.
  // This ensures a malicious page cannot modify the iframe contents.
  pickerDiv = document.createElement('div')
  pickerDiv.id = 'brave-element-picker'
  shadowRoot = pickerDiv.attachShadow({ mode: 'closed' })

  // Will be resolved by webpack to the file content.
  // It's a trusted content so it's safe to use innerHTML.
  // eslint-disable-next-line no-unsanitized/property
  shadowRoot.innerHTML = require('./element_picker.html')

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
    '',
  ].join(' !important;')

  pickerDiv.setAttribute('style', pickerCSSStyle)
  document.documentElement.appendChild(pickerDiv)


  // Setup listeners to assist element picker
  document.addEventListener('keydown', elementPickerOnKeydown, true)
  document.addEventListener('resize', elementPickerViewportChanged)
  document.addEventListener('scroll', elementPickerViewportChanged)

  return shadowRoot
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
  const coords = elems.map((e: Element) => targetRectFromElement(e))
  highlightElements(coords)
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

const elementPickerHoverCoordsChanged = (x: number, y: number) => {
  const elem = elementFromFrameCoords(x, y)
  if (elem instanceof HTMLElement && elem !== lastHoveredElem) {
    recalculateAndSendTargets([elem])
    lastHoveredElem = elem
  }
}

const elementPickerUserSelectedTarget = (specificity: number) => {
  if (lastHoveredElem instanceof HTMLElement) {
    const selector = onTargetSelected(lastHoveredElem, specificity)
    if (selector !== '') {
      recalculateAndSendTargets(Array.from(document.querySelectorAll(selector)))
    }
    return {
      isValid: selector !== '',
      selector: selector.trim(),
    }
  }
  return {
    isValid: false,
    selector: '',
  }
}

const elementPickerUserModifiedRule = (selector: string) => {
  if (selector.length > 0) {
    recalculateAndSendTargets(Array.from(document.querySelectorAll(selector)))
  }
}

const launchElementPicker = (root: ShadowRoot) => {
  let hasSelectedTarget = false

  root.addEventListener(
    'keydown',
    (event: KeyboardEvent) => {
      if (event.key === 'Escape') {
        event.stopPropagation()
        event.preventDefault()
        quitElementPicker()
      }
    },
    true,
  )

  const svg = root.getElementById('picker-ui')!

  svg.addEventListener(
    'mousemove',
    (event) => {
      if (!hasSelectedTarget) {
        elementPickerHoverCoordsChanged(event.clientX, event.clientY)
      }
      event.stopPropagation()
    },
    true,
  )

  const rulesTextArea: HTMLInputElement = root.querySelector(
    '#rules-box > textarea',
  )!
  let textInputTimer: any = null
  rulesTextArea.addEventListener('input', () => {
    clearTimeout(textInputTimer)
    textInputTimer = setTimeout(() => {
      const selector = rulesTextArea.value.trim()
      if (selector.length > 0) {
        elementPickerUserModifiedRule(selector)
      }
    }, 700)
  })
  rulesTextArea.addEventListener('focus', () => {
    hasSelectedTarget = true
    togglePopup(true)
  })
  const section = root.querySelector('section')!
  const togglePopup = (show: boolean) => {
    section.setAttribute('style', `opacity : ${show ? '1' : '0.2'}`)
  }

  const slider = root.getElementById('sliderSpecificity') as HTMLInputElement

  const dispatchSelect = () => {
    const { isValid, selector } = elementPickerUserSelectedTarget(
      parseInt(slider.value),
    )
    if (isValid) {
      hasSelectedTarget = true
      togglePopup(true)
      // disable hovering new elements
      rulesTextArea.value = selector
    }
  }

  slider.addEventListener('input', () => {
    dispatchSelect()
  })

  svg.addEventListener('click', () => {
    if (hasSelectedTarget) {
      // We are already previewing a target. We'll interpet another click
      // as the user wanting back control of the UI.
      hasSelectedTarget = false
      slider.value = '5'
      togglePopup(false)
      return
    }
    dispatchSelect()
  })

  const createButton = root.getElementById('btnCreate')!
  createButton.addEventListener('click', () => {
    const selector = rulesTextArea.value.trim()
    if (selector.length > 0) {
      api.cosmeticFilterCreate(selector)
      quitElementPicker()
    }
  })

  const quitButton = root.getElementById('btnQuit')!
  quitButton.addEventListener('click', () => {
    quitElementPicker()
  })

  const manageButton = root.getElementById('btnManage')!
  manageButton.addEventListener('click', () => {
    api.cosmeticFilterManage();
  })
}

const highlightElements = (coords: TargetRect[]) => {
  if (!shadowRoot) return
  const svg = shadowRoot.getElementById('picker-ui')!
  const svgMask = shadowRoot.getElementById('highlight-mask')!

  // // Delete old element targeting rectangles and their corresponding masks
  const oldMask = svg.querySelectorAll('.mask')
  for (const old of oldMask) {
    old.remove()
  }

  for (const rect of coords) {
    // Add the mask to the SVG definition so the dark background is removed
    const mask = document.createElementNS(NSSVG, 'rect')
    mask.classList.add('mask')
    mask.setAttribute('x', rect.x.toString())
    mask.setAttribute('y', rect.y.toString())
    mask.setAttribute('width', rect.width.toString())
    mask.setAttribute('height', rect.height.toString())
    svgMask.appendChild(mask)

    // Use the same element, but add the target class which turns the
    // target rectangle orange
    const braveTargetingArea = mask.cloneNode() as SVGRectElement
    braveTargetingArea.classList.add('target')
    svg.appendChild(braveTargetingArea)
  }
}

launchElementPicker(attachElementPicker())
