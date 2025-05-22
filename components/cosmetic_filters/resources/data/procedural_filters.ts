// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 *
 *  This file should be kept up to date with
 *  https://github.com/brave-experiments/procedural-filters-js
 *  until the full implementation can be shared with iOS inside
 *  the brave-core repo.
 *
 */

/**
 *
 * src/declarations.d.ts
 *
 * note: `ProceduralOperator` and `ProceduralSelector`
 *       are in components/definitions/global.d.ts
 *
 */

type CSSSelector = string
type CSSInstruction = string
type CSSValue = string

type OperatorType = string
type OperatorArg = CSSSelector | ProceduralSelector | string
type OperatorResult = HTMLElement[]

type UnboundStringFunc = (arg: string, element: HTMLElement) => OperatorResult
type UnboundChildRuleOrStringFunc = (
  arg: string | ProceduralSelector,
  element: HTMLElement) => OperatorResult
type UnboundOperatorFunc = UnboundStringFunc | UnboundChildRuleOrStringFunc
type OperatorFunc = (element: HTMLElement) => OperatorResult

/* post-processed for convenient usage in JS */
interface CompiledProceduralOperator {
  type: OperatorType
  func: OperatorFunc
  args: OperatorArg[]
}
type CompiledProceduralSelector = CompiledProceduralOperator[]

type NeedlePosition = number
type TextMatchRule = (targetText: string, exact?: boolean) => boolean
type KeyValueMatchRules = [
  keyMatchRule: TextMatchRule,
  valueMatchRule: TextMatchRule,
]

/**
 *
 * src/main.ts
 *
 */

const W = window

const _asHTMLElement = (node: Node): HTMLElement | null => {
  return (node instanceof HTMLElement) ? node : null
}

const _compileRegEx = (regexText: string): RegExp => {
  const regexParts = regexText.split('/')
  const regexPattern = regexParts[1]
  const regexArgs = regexParts[2]
  const regex = new W.RegExp(regexPattern, regexArgs)
  return regex
}

// Check to see if the string `value` either
// contains the the string `test` (if `test` does
// not start with `/`) or if the string
// value matches the regex `test`.
// We assume if test isn't a string, its a regex object.
//
// Rules:
//   - if `test` starts with `/` we treat it as a regex
//     literal
//   - if `text` is an empty string, we treat it as
//     matching any case where value is only whitespace
//   - otherwise, check to see if value contains the
//     string `test`
//
// If `exact` is true, then the string case it tested
// for an exact match (the regex case is not affected).
const _testMatches = (test: string, value: string, exact: boolean = false): boolean => {
  if (test[0] === '/') {
    return value.match(_compileRegEx(test)) !== null
  }
  if (test === '') {
    return value.trim() === ''
  }
  if (exact) {
    return value === test
  }
  return value.includes(test)
}

const _extractKeyMatchRuleFromStr = (text: string): [TextMatchRule, number] => {
  const quotedTerminator = '"='
  const unquotedTerminator = '='
  const isQuotedCase = text[0] === '"'

  const [terminator, needlePosition] = isQuotedCase
    ? [quotedTerminator, 1]
    : [unquotedTerminator, 0]

  const indexOfTerminator = text.indexOf(terminator, needlePosition)
  if (indexOfTerminator === -1) {
    throw new Error(
      `Unable to parse key rule from ${text}. Key rule starts with `
      + `${text[0]}, but doesn't include '${terminator}'`)
  }

  const testCaseStr = text.slice(needlePosition, indexOfTerminator)
  const testCaseFunc = _testMatches.bind(undefined, testCaseStr)
  const finalNeedlePosition = indexOfTerminator + terminator.length
  return [testCaseFunc, finalNeedlePosition]
}

const _extractValueMatchRuleFromStr = (text: string,
                                       needlePosition = 0): TextMatchRule => {
  const isQuotedCase = text[needlePosition] === '"'
  let endIndex

  if (isQuotedCase) {
    if (text.at(-1) !== '"') {
      throw new Error(
        `Unable to parse value rule from ${text}. Value rule starts with `
        + '" but doesn\'t end with "')
    }
    needlePosition += 1
    endIndex = text.length - 1
  }
  else {
    endIndex = text.length
  }

  const testCaseStr = text.slice(needlePosition, endIndex)
  const testCaseFunc = _testMatches.bind(undefined, testCaseStr)
  return testCaseFunc
}

// Parse an argument like `"abc"="xyz"` into
// a test for the key, and a test for the value.
// This will return two functions then, that you
// should use for checking the key and values
// in your test case.
//
// const key = ..., value = ...
// const [keyTestFunc, valueTestFunc] = _parseKeyValueMatchArg(arg)
//
// if (keyTestFunc(key))) {
//   // key matches the test condition
// }
const _parseKeyValueMatchRules = (arg: string): KeyValueMatchRules => {
  const [keyMatchRule, needlePos] = _extractKeyMatchRuleFromStr(arg)
  const valueMatchRule = _extractValueMatchRuleFromStr(arg, needlePos)
  return [keyMatchRule, valueMatchRule]
}

const _parseCSSInstruction = (arg: string): [CSSInstruction, CSSValue] => {
  const rs = arg.split(':')
  if (rs.length !== 2) {
    throw Error(`Unexpected format for a CSS rule: ${arg}`)
  }
  return [rs[0].trim(), rs[1].trim()]
}

const _allOtherSiblings = (element: HTMLElement): HTMLElement[] => {
  if (!element.parentNode) {
    return []
  }
  const siblings = Array.from(element.parentNode.children)
  const otherHTMLElements = []
  for (const sib of siblings) {
    if (sib === element) {
      continue
    }
    const siblingHTMLElement = _asHTMLElement(sib)
    if (siblingHTMLElement !== null) {
      otherHTMLElements.push(siblingHTMLElement)
    }
  }
  return otherHTMLElements
}

const _nextSiblingElement = (element: HTMLElement): HTMLElement | null => {
  if (!element.parentNode) {
    return null
  }
  const siblings = W.Array.from(element.parentNode.children)
  const indexOfElm = siblings.indexOf(element)
  const nextSibling = siblings[indexOfElm + 1]
  if (nextSibling === undefined) {
    return null
  }
  return _asHTMLElement(nextSibling)
}

const _allChildren = (element: HTMLElement): HTMLElement[] => {
  return W.Array.from(element.children)
    .map(e => _asHTMLElement(e))
    .filter(e => e !== null) as HTMLElement[]
}

const _allChildrenRecursive = (element: HTMLElement): HTMLElement[] => {
  return W.Array.from(element.querySelectorAll(':scope *'))
    .map(e => _asHTMLElement(e))
    .filter(e => e !== null) as HTMLElement[]
}

const _stripCssOperator = (operator: string, selector: string) => {
  if (selector[0] !== operator) {
    throw new Error(
      `Expected to find ${operator} in initial position of "${selector}`)
  }
  return selector.replace(operator, '').trimStart()
}

// Implementation of ":css-selector" rule
const operatorCssSelector = (selector: CSSSelector,
                             element: HTMLElement): OperatorResult => {
  const trimmedSelector = selector.trimStart()
  if (trimmedSelector.startsWith('+')) {
    const subOperator = _stripCssOperator('+', trimmedSelector)
    if (subOperator === null) {
      return []
    }
    const nextSibNode = _nextSiblingElement(element)
    if (nextSibNode === null) {
      return []
    }
    return nextSibNode.matches(subOperator) ? [nextSibNode] : []
  }
  else if (trimmedSelector.startsWith('~')) {
    const subOperator = _stripCssOperator('~', trimmedSelector)
    if (subOperator === null) {
      return []
    }
    const allSiblingNodes = _allOtherSiblings(element)
    return allSiblingNodes.filter(x => x.matches(subOperator))
  }
  else if (trimmedSelector.startsWith('>')) {
    const subOperator = _stripCssOperator('>', trimmedSelector)
    if (subOperator === null) {
      return []
    }
    const allChildNodes = _allChildren(element)
    return allChildNodes.filter(x => x.matches(subOperator))
  }
  else if (selector.startsWith(' ')) {
    return Array.from(element.querySelectorAll(':scope ' + trimmedSelector))
  }

  if (element.matches(selector)) {
    return [element]
  }
  return []
}

const _hasPlainSelectorCase = (selector: CSSSelector,
                               element: HTMLElement): OperatorResult => {
  return element.matches(selector) ? [element] : []
}

const _hasProceduralSelectorCase = (selector: ProceduralSelector,
                                    element: HTMLElement): OperatorResult => {
  const shouldBeGreedy = selector[0]?.type !== 'css-selector'
  const initElements = shouldBeGreedy
    ? _allChildrenRecursive(element)
    : [element]
  const matches = compileAndApplyProceduralSelector(selector, initElements)
  return matches.length === 0 ? [] : [element]
}

// Implementation of ":has" rule
const operatorHas = (instruction: CSSSelector | ProceduralSelector,
                     element: HTMLElement): OperatorResult => {
  if (W.Array.isArray(instruction)) {
    return _hasProceduralSelectorCase(instruction, element)
  }
  else {
    return _hasPlainSelectorCase(instruction, element)
  }
}

// Implementation of ":has-text" rule
const operatorHasText = (instruction: string,
                         element: HTMLElement): OperatorResult => {
  const text = element.innerText
  const valueTest = _extractValueMatchRuleFromStr(instruction)
  return valueTest(text) ? [element] : []
}

const _notPlainSelectorCase = (selector: CSSSelector,
                               element: HTMLElement): OperatorResult => {
  return element.matches(selector) ? [] : [element]
}

const _notProceduralSelectorCase = (selector: ProceduralSelector,
                                    element: HTMLElement): OperatorResult => {
  const matches = compileAndApplyProceduralSelector(selector, [element])
  return matches.length === 0 ? [element] : []
}

// Implementation of ":not" rule
const operatorNot = (instruction: CSSSelector | ProceduralSelector,
                     element: HTMLElement): OperatorResult => {
  if (Array.isArray(instruction)) {
    return _notProceduralSelectorCase(instruction, element)
  }
  else {
    return _notPlainSelectorCase(instruction, element)
  }
}

// Implementation of ":matches-property" rule
const operatorMatchesProperty = (instruction: string,
                                 element: HTMLElement): OperatorResult => {
  const [keyTest, valueTest] = _parseKeyValueMatchRules(instruction)
  for (const [propName, propValue] of Object.entries(element)) {
    if (!keyTest(propName)) {
      continue
    }
    if (!valueTest(propValue)) {
      continue
    }
    return [element]
  }
  return []
}

// Implementation of ":min-text-length" rule
const operatorMinTextLength = (instruction: string,
                               element: HTMLElement): OperatorResult => {
  const minLength = +instruction
  if (minLength === W.NaN) {
    throw new Error(`min-text-length: Invalid arg, ${instruction}`)
  }
  return element.innerText.trim().length >= minLength ? [element] : []
}

// Implementation of ":matches-attr" rule
const operatorMatchesAttr = (instruction: string,
                             element: HTMLElement): OperatorResult => {
  const [keyTest, valueTest] = _parseKeyValueMatchRules(instruction)
  for (const attrName of element.getAttributeNames()) {
    if (!keyTest(attrName)) {
      continue
    }
    const attrValue = element.getAttribute(attrName)
    if (attrValue === null || !valueTest(attrValue)) {
      continue
    }
    return [element]
  }
  return []
}

// Implementation of ":matches-css-*" rules
const operatorMatchesCSS = (beforeOrAfter: string | null,
                            cssInstruction: string,
                            element: HTMLElement): OperatorResult => {
  const [cssKey, expectedVal] = _parseCSSInstruction(cssInstruction)
  const elmStyle = W.getComputedStyle(element, beforeOrAfter)
  const styleValue = elmStyle.getPropertyValue(cssKey)
  if (styleValue === undefined) {
    // We're querying for a style property that doesn't exist, which
    // trivially doesn't match then.
    return []
  }
  return expectedVal === styleValue ? [element] : []
}

// Implementation of ":matches-media" rule
const operatorMatchesMedia = (instruction: string,
                              element: HTMLElement): OperatorResult => {
  return W.matchMedia(instruction).matches ? [element] : []
}

// Implementation of ":matches-path" rule
const operatorMatchesPath = (instruction: string,
                             element: HTMLElement): OperatorResult => {
  const pathAndQuery = W.location.pathname + W.location.search
  const matchRule = _extractValueMatchRuleFromStr(instruction)
  return matchRule(pathAndQuery) ? [element] : []
}

const _upwardIntCase = (intNeedle: NeedlePosition,
                        element: HTMLElement): OperatorResult => {
  if (intNeedle < 1 || intNeedle >= 256) {
    throw new Error(`upward: invalid arg, ${intNeedle}`)
  }
  let currentElement: HTMLElement | ParentNode | null = element
  while (currentElement !== null && intNeedle > 0) {
    currentElement = currentElement.parentNode
    intNeedle -= 1
  }
  if (currentElement === null) {
    return []
  }
  else {
    const htmlElement = _asHTMLElement(currentElement)
    return (htmlElement === null) ? [] : [htmlElement]
  }
}

const _upwardProceduralSelectorCase = (selector: ProceduralSelector,
                                       element: HTMLElement): OperatorResult => {
  const childFilter = compileProceduralSelector(selector)
  let needle: ParentNode | HTMLElement | null = element
  while (needle !== null) {
    const currentElement = _asHTMLElement(needle)
    if (currentElement === null) {
      break
    }
    const matches = applyCompiledSelector(childFilter, [currentElement])
    if (matches.length !== 0) {
      return [currentElement]
    }
    needle = currentElement.parentNode
  }
  return []
}

const _upwardPlainSelectorCase = (selector: CSSSelector,
                                  element: HTMLElement): OperatorResult => {
  let needle: ParentNode | HTMLDocument | null = element
  while (needle !== null) {
    const currentElement = _asHTMLElement(needle)
    if (currentElement === null) {
      break
    }
    if (currentElement.matches(selector)) {
      return [currentElement]
    }
    needle = currentElement.parentNode
  }
  return []
}

// Implementation of ":upward" rule
const operatorUpward = (instruction: string | ProceduralSelector,
                        element: HTMLElement): OperatorResult => {
  if (W.Number.isInteger(+instruction)) {
    return _upwardIntCase(+instruction, element)
  }
  else if (W.Array.isArray(instruction)) {
    return _upwardProceduralSelectorCase(instruction, element)
  }
  else {
    return _upwardPlainSelectorCase(instruction, element)
  }
}

// Implementation of ":xpath" rule
const operatorXPath = (instruction: string,
                       element: HTMLElement): HTMLElement[] => {
  const result = W.document.evaluate(instruction, element, null,
                                     W.XPathResult.UNORDERED_NODE_ITERATOR_TYPE,
                                     null)
  const matches: HTMLElement[] = []
  let currentNode: Node | null
  while ((currentNode = result.iterateNext())) {
    const currentElement = _asHTMLElement(currentNode)
    if (currentElement !== null) {
      matches.push(currentElement)
    }
  }
  return matches
}

const ruleTypeToFuncMap: Record<OperatorType, UnboundOperatorFunc> = {
  'contains': operatorHasText,
  'css-selector': operatorCssSelector,
  'has': operatorHas,
  'has-text': operatorHasText,
  'matches-attr': operatorMatchesAttr,
  'matches-css': operatorMatchesCSS.bind(undefined, null),
  'matches-css-after': operatorMatchesCSS.bind(undefined, '::after'),
  'matches-css-before': operatorMatchesCSS.bind(undefined, '::before'),
  'matches-media': operatorMatchesMedia,
  'matches-path': operatorMatchesPath,
  'matches-property': operatorMatchesProperty,
  'min-text-length': operatorMinTextLength,
  'not': operatorNot,
  'upward': operatorUpward,
  'xpath': operatorXPath,
}

const compileProceduralSelector = (operators: ProceduralSelector): CompiledProceduralSelector => {
  const outputOperatorList = []
  for (const operator of operators) {
    const anOperatorFunc = ruleTypeToFuncMap[operator.type]
    const args = [operator.arg]
    if (anOperatorFunc === undefined) {
      throw new Error(`Not sure what to do with operator of type ${operator.type}`)
    }

    outputOperatorList.push({
      type: operator.type,
      func: anOperatorFunc.bind(undefined, ...args),
      args,
    })
  }

  return outputOperatorList
}

// List of operator types that will be either globally true or false
// independent of the passed element. We use this list to optimize
// applying each operator (i.e., we just check the first element, and then
// accept or reject all elements in the consideration set accordingly).
const fastPathOperatorTypes: OperatorType[] = [
  'matches-media',
  'matches-path',
]

const _determineInitNodesAndIndex = (selector: CompiledProceduralSelector,
                                    initNodes?: HTMLElement[]): [number, HTMLElement[]] => {
  let nodesToConsider: HTMLElement[] = []
  let index = 0

  // A couple of special cases to consider.
  //
  // Case one: we're applying the procedural filter on a set of nodes (instead
  // of the entire document)  In this case, we already know which nodes to
  // consider, easy case.
  const firstOperator = selector[0]
  const firstOperatorType = firstOperator.type
  const firstArg = firstOperator.args[0]

  if (initNodes !== undefined) {
    nodesToConsider = W.Array.from(initNodes)
  }
  else if (firstOperatorType === 'css-selector') {
    const selector = firstArg as CSSSelector
    // Case two: we're considering the entire document, and the first operator
    // is a 'css-selector'. Here, we just special case using querySelectorAll
    // instead of starting with the full set of possible nodes.
    nodesToConsider = W.Array.from(W.document.querySelectorAll(selector))
    index += 1
  }
  else if (firstOperatorType === 'xpath') {
    const xpath = firstArg as string
    nodesToConsider = operatorXPath(xpath, W.document.documentElement)
    index += 1
  }
  else {
    // Case three: we gotta apply the first operator to the entire document.
    // Yuck but un-avoidable.
    const allNodes = W.Array.from(W.document.all)
    nodesToConsider = allNodes.filter(_asHTMLElement) as HTMLElement[]
  }
  return [index, nodesToConsider]
}

const applyCompiledSelector = (selector: CompiledProceduralSelector,
                               initNodes?: HTMLElement[]): HTMLElement[] => {
  const initState = _determineInitNodesAndIndex(selector, initNodes)
  let [index, nodesToConsider] = initState
  const numOperators = selector.length
  for (index; nodesToConsider.length > 0 && index < numOperators; ++index) {
    const operator = selector[index]
    const operatorFunc = operator.func
    const operatorType = operator.type

    // Note that we special case the :matches-path case here, since if
    // if it passes for one element, then it will pass for all elements.
    if (fastPathOperatorTypes.includes(operatorType)) {
      const firstNode = nodesToConsider[0]
      if (operatorFunc(firstNode).length === 0) {
        nodesToConsider = []
      }
      // Note that unless we've taken the if-true branch above, then
      // the nodesToConsider array will still have all the elements
      // it started with.
      break
    }

    let newNodesToConsider: HTMLElement[] = []
    for (const aNode of nodesToConsider) {
      const result = operatorFunc(aNode)
      newNodesToConsider = newNodesToConsider.concat(result)
    }
    nodesToConsider = newNodesToConsider
  }

  return nodesToConsider
}

const compileAndApplyProceduralSelector = (selector: ProceduralSelector,
                                           initElements: HTMLElement[]): HTMLElement[] => {
  const compiled = compileProceduralSelector(selector)
  return applyCompiledSelector(compiled, initElements)
}

export {
  applyCompiledSelector,
  compileProceduralSelector,
  compileAndApplyProceduralSelector,
}
