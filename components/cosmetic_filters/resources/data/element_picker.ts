// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
const NSSVG = 'http://www.w3.org/2000/svg'

let pickerDiv: HTMLDivElement | null
let shadowRoot: ShadowRoot | null
const isAndroid = /(android)/i.test(navigator.userAgent)

const api = {
  cosmeticFilterCreate: (selector: string) => {
    cf_worker.addSiteCosmeticFilter(selector)
  },
  cosmeticFilterManage: () => {
    cf_worker.manageCustomFilters()
  },
  getElementPickerThemeInfo: () => {
    cf_worker.getElementPickerThemeInfo()
  }
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
  recalculateAndSendTargets(null)
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
  shadowRoot.innerHTML = isAndroid ?
    require('./android_element_picker.html') :
    require('./element_picker.html')

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
  window.addEventListener('resize', elementPickerViewportChanged)
  window.addEventListener('scroll', elementPickerViewportChanged)

  return shadowRoot
}

interface TargetRect {
  x: number
  y: number
  width: number
  height: number
}

class Target {
  element: Element
  rectElem: Element
  coord: TargetRect

  constructor (elem: Element) {
    this.element = elem
    this.coord = targetRectFromElement(this.element)
  }

  forceRecalcCoords() {
    this.coord = targetRectFromElement(this.element)
  }
}

class TargetsCollection {
  targets: Target[] = []
  togglePicker: ((val: boolean) => void) | null = null
  reset(elems: Element[]) {
    this.targets.length = 0
    elems.forEach((elem: Element) => {
      this.targets.push(new Target(elem))
    });
  }

  getXpathsForMarked(): string[] {
    return this.targets.map((el) => getElementXpath(el.element))
      .filter((item): item is string => item !== null)
  }

  forceRecalcCoords() {
    this.targets.forEach(t => t.forceRecalcCoords())
    // for case when element no longer in the DOM
    this.targets = this.targets.filter(item =>
        item.coord.height !== 0 && item.coord.width !== 0)
    if(this.targets.length === 0 && this.togglePicker) {
      this.togglePicker(false)
    }
  }

  size() {
    return this.targets.length
  }
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
const targetedElems = new TargetsCollection

const recalculateAndSendTargets = (elems: Element[] | null) => {
  if(elems) {
    targetedElems.reset(elems)
  } else {
    targetedElems.forceRecalcCoords()
  }

  highlightElements()
}

const hideByCssSelector = (selector: string) => {
  const styleId = 'brave-content-picker-style'
  let style = document.getElementById(styleId)
  if (!style) {
    style = document.createElement('style')
    style.id = styleId
    document.head.appendChild(style)
  }
  style.innerText += `${selector} {display: none !important;}`
}

const hideByXPath = (xpath: string): void => {
  const hideStyle: Partial<CSSStyleDeclaration> = {
    display: 'none'
  }
  const result = document.evaluate(
      xpath,
      document,
      null,
      XPathResult.ORDERED_NODE_ITERATOR_TYPE,
      null
  );

  let node: Node | null = result.iterateNext();

  while (node) {
      if (node instanceof HTMLElement) {
          Object.assign(node.style, hideStyle);
      }
      node = result.iterateNext();
  }
}

const getElementByXpath = (xpath: string) => {
  const result = document.evaluate(
    xpath,
    document,
    null,
    XPathResult.FIRST_ORDERED_NODE_TYPE,
    null
  );

  // Return the single node found, or null if none
  return result.singleNodeValue as Element | null;
}

const getElementXpath = (element: Element | null): string => {
  if (!element || !(element instanceof Element)) {
    return ''
  }

  const getXPathSegment = (el: Element): string => {
      const tagName = el.tagName.toLowerCase();

      if (!el.parentElement) {
          return `/${tagName}`
      }

      const siblings = Array.from(el.parentElement.children).filter(
          (sibling) => sibling.tagName === el.tagName
      );
      if (siblings.length === 1) {
          return `/${tagName}`
      }

      const index = siblings.indexOf(el) + 1
      return `/${tagName}[${index}]`
  };

  const segments: string[] = []

  while (element) {
      segments.unshift(getXPathSegment(element))
      element = element.parentElement!
  }

  return segments.join('')
}

const onTargetSelected = (selected: Element | null, index: number): string => {
  if (lastHoveredElem === null) { return '' }

  if (isAndroid) {
    return getElementXpath(selected)
  }

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
    try {
      if ((mask & SpecificityFlags.Id) && b.hasId ||
          document.querySelectorAll(b.toString(mask)).length === 1) {
        break
      }
    } catch {
      continue
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

const getElementBySelector = (selector: string) => {
  let elements: Element[] | null;
  if (isAndroid) {
      const singleElement = getElementByXpath(selector);
      elements = singleElement ? [singleElement] : null;
  } else {
      const nodeList = document.querySelectorAll(selector);
      elements = nodeList.length > 0 ? Array.from(nodeList) : null;
  }
  return elements
}

const elementPickerUserSelectedTarget = (specificity: number) => {
  if (lastHoveredElem instanceof HTMLElement) {
    const selector = onTargetSelected(lastHoveredElem, specificity)
    if (selector !== '') {
      try {
        recalculateAndSendTargets(getElementBySelector(selector))
      } catch {}
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
    try {
      recalculateAndSendTargets(Array.from(document.querySelectorAll(selector)))
    } catch {}
  }
}

const launchElementPicker = (root: ShadowRoot) => {
  let hasSelectedTarget = false

  const btnShowRulesBox = root.getElementById('btnShowRulesBox')
  if (isAndroid && btnShowRulesBox) {
    btnShowRulesBox.style.display = 'none'
  }

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
      if (isAndroid) {
        return
      }
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

  const section = root.getElementById('main-section')!
  const togglePopup = (show: boolean) => {
    if(isAndroid) {
      if (show) {
        createButton.classList.remove('block-button-disabled')
        createButton.textContent = "Block Element"
      } else {
        createButton.classList.add('block-button-disabled')
        createButton.textContent = "Select element you want to block"
      }
      return
    }
    section.style.setProperty('opacity', show ? '1' : '0.2')
  }
  targetedElems.togglePicker = togglePopup

  const slider = root.getElementById('sliderSpecificity') as HTMLInputElement
  if (isAndroid) {
    slider.style.display = 'none'
  }

  const prefersDarkScheme = window.matchMedia('(prefers-color-scheme: dark)');
  const handleColorSchemeChange = () => {
    api.getElementPickerThemeInfo()
  };
  prefersDarkScheme.addEventListener('change', handleColorSchemeChange);
  window.content_cosmetic.setTheme = (bgcolor: number) => {
    const colorHex = `#${(bgcolor & 0xFFFFFF).toString(16).padStart(6, '0')}`
    section.style.setProperty('background-color', colorHex)
    root.querySelectorAll('.secondary-button').forEach(e =>
        (e as HTMLElement).style.setProperty('background-color', colorHex))
  }
  handleColorSchemeChange()

  const dispatchSelect = () => {
    const { isValid, selector } = elementPickerUserSelectedTarget(
      parseInt(slider.value),
    )

    hasSelectedTarget = isValid
    togglePopup(isValid)
    if (isValid) {
      rulesTextArea.value = selector
    } else {
      slider.value = '4'
    }
  }

  slider.addEventListener('input', () => {
    dispatchSelect()
  })

  const oneClickEventHandler = (event: MouseEvent | TouchEvent) => {
    let elem: Element | null = null
    if (event instanceof MouseEvent) {
      elem = elementFromFrameCoords(event.clientX, event.clientY)
    } else if (event instanceof TouchEvent){
      const touch = event.touches[0];
      elem = elementFromFrameCoords(touch.clientX, touch.clientY)
    }

    if (elem) {
      recalculateAndSendTargets([elem])
      lastHoveredElem = elem as HTMLElement
    }

    dispatchSelect()
  }

  svg.addEventListener('click', oneClickEventHandler)

  const createButton = root.getElementById('btnCreate')!
  createButton.addEventListener('click', () => {
    if (createButton.classList.contains('block-button-disabled')) {
      return
    }
    if (isAndroid) {
      const selectedXpaths = targetedElems.getXpathsForMarked()
      if(selectedXpaths && selectedXpaths.length > 0) {
        for(const expr of selectedXpaths) {
          const rule = `:xpath(${expr})`
          api.cosmeticFilterCreate(rule)
          hideByXPath(expr)
        }
        quitElementPicker()
      }
      return
    }
    const selector = rulesTextArea.value.trim()
    if (selector.length > 0) {
      api.cosmeticFilterCreate(selector)
      hideByCssSelector(selector)
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

  const toggleDisplay = (target: HTMLElement | null,
    trigger: HTMLElement | null) => {
    if(!target || !trigger){
      return
    }
    trigger.addEventListener('click', e => {
      if (target.style.display !== 'block') {
        target.style.display = 'block'
        trigger.textContent = 'Hide rules'
      } else {
        target.style.display = 'none'
        trigger.textContent = 'Show rules'
      }
    })
  }
  const rulesBox = root.getElementById('rules-box')!
  const showRulesButton = root.getElementById('btnShowRulesBox')!
  toggleDisplay(rulesBox, showRulesButton)
}

const highlightElements = () => {
  if (!shadowRoot) return
  const svg = shadowRoot.getElementById('picker-ui')!
  const svgMask = shadowRoot.getElementById('highlight-mask')!

  svg.querySelectorAll('.mask').forEach(el => el.remove());

  const svgMaskFragment = document.createDocumentFragment();
  const svgFragment = document.createDocumentFragment();

  const createMaskElement = (): SVGRectElement => {
    const mask = document.createElementNS(NSSVG, 'rect')
    mask.classList.add('mask')
    mask.rx.baseVal.value = 10
    mask.setAttribute('px', '10px')
    mask.setAttribute('stroke-linejoin', 'round')
    return mask
  }

  for (const target of targetedElems.targets) {
    // Add the mask to the SVG definition so the dark background is removed
    const mask = createMaskElement()
    mask.x.baseVal.value = target.coord.x;
    mask.y.baseVal.value = target.coord.y;
    mask.width.baseVal.value = target.coord.width;
    mask.height.baseVal.value = target.coord.height;
    mask.rx.baseVal.value = 10;
    svgMaskFragment.appendChild(mask)

    // Use the same element, but add the target class which turns the
    // target rectangle orange
    const targetingArea = mask.cloneNode(false) as SVGRectElement
    targetingArea.classList.add('target')
    target.rectElem = targetingArea

    svgFragment.appendChild(targetingArea)
  }
  svgMask.appendChild(svgMaskFragment)
  svg.appendChild(svgFragment)
}

const active = document.getElementById('brave-element-picker')
if (!active) {
  launchElementPicker(attachElementPicker())
}
