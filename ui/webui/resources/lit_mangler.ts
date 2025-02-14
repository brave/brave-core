// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { type TemplateResult } from "//resources/lit/v3_0/lit.rollup.js";

type StringResult = string | false | undefined | null | number | (() => string | false | undefined | null | number)
type NodeResult = string | false | undefined | null | number | TemplateResult
    | (() => string | false | undefined | null | number | TemplateResult)

// Supports the following tags:
// 1. $$lit_mangler_{n}$$ - a literal replacement. Needed for no stringifyable values
// 2. #$$lit_mangler_{n}$$ - a tagged template literal replacement opening tag. Content between this and the closing tag will be parsed as a TemplateResult
// 3. /$$lit_mangler_{n}$$ - a tagged template literal replacement closing tag.
// 4. #$$lit_mangler_{n}$${i} - an opening tag for an item in an array
// 5. /$$lit_mangler_{n}$${i} - closing tag for an item in an array
const placeholderRegex = /(?<prefix>\/|#)?\$\$lit_mangler_(?<id>\d+)(_(?<index>\d+))?\$\$/gm

// This is what LitHtml uses to identify a template result
const templateTag = '_$litType$'

// Used to parse the TemplateResult into a DOM object we can manipulate.
const parser = new DOMParser()

const trustedTypesPolicy = window.trustedTypes!.createPolicy('lit-mangler', {
    createHTML: (str: string) => str
})

// This represents a placeholder element in the mangled HTML we use for running
// queries on.
type Placeholder = {
    id: number,
    start: number,
    end: number,
} & ({
    type: 'literal'
} | {
    type: 'pair',
    valueStart: number,
    valueEnd: number,
} | {
    type: "array",
    items: Placeholder[]
})

/**
 * Finds the next placeholder tag in the html.
 * @param rawHtml The html to search
 * @param start The index to start searching from
 * @param end The index to end searching at (result will be null if it goes past this).
 * @returns The next placeholder in the html, or null if no more placeholders are found
 */
const findNextTag = (rawHtml: string, start: number, end: number): Placeholder | null => {
    placeholderRegex.lastIndex = start
    const match = placeholderRegex.exec(rawHtml)

    if (!match) return null

    const tagEnd = match.index + match[0].length

    // Don't allow parsing outside the range of the tag.
    if (tagEnd > end) return null

    const id = parseInt(match.groups!['id'])

    if (isNaN(id)) throw new Error(`Failed to parse id from ${match[0]}`)

    if (match[0].startsWith('#')) {
        // Closing tag is the same as the opening tag but starting with a slash
        const closingTag = match[0].replace('#', '/')
        const closingIndex = rawHtml.indexOf(closingTag, tagEnd)

        if (closingIndex === -1) {
            throw new Error(`Couldn't find closing tag "${closingTag}"`)
        }

        // Try and parse the arrayIndex - if this is just an object this will be NaN
        const arrayIndex = parseInt(match.groups!['index'])

        // First item in an array - parse all subsequent items
        if (arrayIndex === 0) {
            let tag: Placeholder | null

            // List of items, with the first one appended
            const items: Placeholder[] = [{
                start: match.index,
                end: closingIndex + closingTag.length,
                valueStart: tagEnd,
                valueEnd: closingIndex,
                type: 'pair',
                id
            }]

            // Keep parsing tags until we find one with a different id
            while (tag = findNextTag(rawHtml, items.at(-1)!.end, end)) {
                if (tag.id !== id) break

                items.push(tag)
            }

            return {
                start: match.index,
                end: items.at(-1)!.end,
                items,
                type: 'array',
                id
            }
        }

        if (match.index >= closingIndex + closingTag.length) throw new Error(`End must be after start! ${closingTag}`)

        return {
            start: match.index,
            end: closingIndex + closingTag.length,
            valueStart: tagEnd,
            valueEnd: closingIndex,

            id,

            type: 'pair'
        }
    }

    return {
        start: match?.index,
        end: tagEnd,
        id,
        type: 'literal'
    }
}

const isStringLiteral = (template: TemplateResult) => {
    return template.strings.length === 1
}

/**
 * Converts a placeholder into a value to be interpolated into the TemplateResult.
 * @param values The values from the original TemplateResult + any new values added by the mangler.
 * @param placeholder The placeholder to convert.
 * @param rawHTML The original html.
 * @returns The value to be interpolated.
 */
const getPlaceholderValue = (values: unknown[], placeholder: Placeholder, rawHTML: string): unknown => {
    if (placeholder.type === 'literal') {
        return values[placeholder.id]
    }

    if (placeholder.type === 'pair') {
        const template = parseTemplate(rawHTML, values, placeholder.valueStart, placeholder.valueEnd)
        if (isStringLiteral(template)) {
            const literal = template.strings[0]

            for (const [re, func] of Object.entries(primitiveDeserializers)) {
                const match = new RegExp(re).exec(literal)
                if (match) return func(match)
            }

            return literal
        }
        return template
    }

    if (placeholder.type === 'array') {
        return placeholder.items.map(p => getPlaceholderValue(values, p, rawHTML))
    }

    return null
}

/**
 * Parses the html and converts it into a TemplateResult.
 * @param rawHTML The html to parse. This should include all the mangler tags
 * @param availableValues The values from the original TemplateResult + any new values added by the mangler.
 * @param start The index to start parsing from. Used for recursive template parsing.
 * @param end The index to end parsing at. Used for recursive template parsing.
 */
export const parseTemplate = (rawHTML: string, availableValues: unknown[], start = 0, end = rawHTML.length): TemplateResult => {
    const strings: string[] = []
    const values: unknown[] = []

    let tag: Placeholder | null
    let parsedTo = start
    while (tag = findNextTag(rawHTML, parsedTo, end)) {
        strings.push(rawHTML.substring(parsedTo, tag.start))
        values.push(getPlaceholderValue(availableValues, tag, rawHTML))

        parsedTo = tag.end
    }

    strings.push(rawHTML.substring(parsedTo, end));
    (strings as any)['raw'] = [...strings]

    return {
        strings: strings as any,
        values,
        _$litType$: 1
    }
}

const primitiveSerializers: { [key: string]: (value: unknown) => string } = {
    'boolean': value => `/Boolean(${value})/`,
    'number': value => `/Number(${value})/`,
}

const primitiveDeserializers: { [key: string]: (match: RegExpMatchArray) => unknown } = {
    '/Boolean\\(true\\)/': () => true,
    '/Boolean\\(false\\)/': () => false,
    '/Number\\((\\d+)\\)/': (match: RegExpMatchArray) => parseFloat(match[1]),
}


/**
 * A class that converts a TemplateResult into a DOMIsh object.
 * This is used to allow us to run queries on the DOM and replace values.
 * 
 * It should not be exposed to outside consumers of this code (they should go
 * through the mangle function and use ManglerElement and ManglerNode instead).
 */
class DOMIsh {
    template: TemplateResult
    root: ManglerElement

    values: unknown[] = []

    #internalRoot: Element


    getPlaceholderForValue(value: unknown) {
        let index = this.values.indexOf(value)
        if (index === -1) {
            index = this.values.length
            this.values.push(value)
        }

        const type = typeof value
        if (type !== 'object' && type !== 'function' && value !== undefined && value !== null) {
            const serializer = primitiveSerializers[type] ?? ((v: unknown) => v)
            return `#$$lit_mangler_${index}$$${serializer(value)}/$$lit_mangler_${index}$$`
        }

        if (type === 'object' && value !== null && templateTag in (value as any)) {
            return `#$$lit_mangler_${index}$$
${this.getRawHTML(value as any)}
/$$lit_mangler_${index}$$`
        }

        if (Array.isArray(value)) {
            return value.map((v, i) => `#$$lit_mangler_${index}_${i}$$
${this.getRawHTML(v as any)}
/$$lit_mangler_${index}_${i}$$`).join('')
        }

        // We use this for placeholding values which can't be represented nicely
        // in HTML, like functions, undefined, null ect.
        return `$$lit_mangler_${index}$$`
    }

    constructor(template: TemplateResult) {
        this.template = template

        const [internal, element] = this.parseFromTemplate(template)
        this.#internalRoot = internal
        this.root = element
    }

    // Note: The parsed result is wrapped in a body
    parseFromTemplate(template: TemplateResult): [HTMLElement, ManglerElement] {
        const rawHTML = this.getRawHTML(template)
        const dom = parser.parseFromString(trustedTypesPolicy!.createHTML(rawHTML) as any, 'text/html')
        return [dom.body, new ManglerElement(dom.body, this)]
    }

    getRawHTML(template: TemplateResult): string {
        return template.strings.reduce((prev, next, currentIndex) => {
            const value = currentIndex < template.values.length
                ? this.getPlaceholderForValue(template.values[currentIndex])
                : ''
            return prev + next + value
        }, '')
    }

    #toTemplateResult(rawHTML: string): TemplateResult {
        return parseTemplate(rawHTML, this.values)
    }

    toTemplateResult(): TemplateResult {
        const rawHTML = this.#internalRoot.innerHTML
        return this.#toTemplateResult(rawHTML)
    }
}

class ManglerNode {
    get parentNode() {
        return this.#node.parentNode && new ManglerNode(this.#node.parentNode, this.domish)
    }

    get parentElement() {
        return this.#node.parentElement && new ManglerElement(this.#node.parentElement, this.domish)
    }

    get textContent(): string | null {
        return this.#node.textContent
    }

    set textContent(value: StringResult) {
        const replacement = this.domish.getPlaceholderForValue(value)
        this.#node.textContent = replacement
    }

    get childNodes() {
        return Array.from(this.#node.childNodes).map(n => new ManglerNode(n, this.domish))
    }

    #node: Node
    protected domish: DOMIsh

    constructor(node: Node, domish: DOMIsh) {
        this.#node = node
        this.domish = domish
    }

    remove() {
        this.#node.parentNode?.removeChild(this.#node)
    }
}

class ManglerElement extends ManglerNode {
    get classList() {
        return {
            // Note: We only add classes as TemplateLiterals
            add: (...tokens: StringResult[]) => {
                this.#element.classList.add(...tokens.map(t => this.domish.getPlaceholderForValue(t)))
            },

            // Note: We want to be able to remove literals from the classList or template values
            remove: (...tokens: StringResult[]) => {
                this.#element.classList.remove(...tokens.flatMap(t => [t, this.domish.getPlaceholderForValue(t)]).filter(t => typeof t === 'string'))
            },

            // If the classList contains the literal token, we'll toggle that. Otherwise,
            // toggle the template values.
            toggle: (token: StringResult) => {
                if (typeof token === 'string' && this.#element.classList.contains(token)) {
                    this.#element.classList.toggle(token)
                    return
                }
                const value = this.domish.getPlaceholderForValue(token)
                this.#element.classList.toggle(value)
            },

            // Determine if the classList contains the literal token or the template value
            contains: (token: StringResult) => {
                const value = typeof token === 'string' ? token : this.domish.getPlaceholderForValue(token)
                return typeof token === 'string' && this.#element.classList.contains(value)
                    || this.#element.classList.contains(this.domish.getPlaceholderForValue(token))
            }
        }
    }

    get children() {
        return Array.from(this.#element.children).map(c => new ManglerElement(c, this.domish))
    }

    #element: Element

    constructor(element: Element, domish: DOMIsh) {
        super(element, domish)

        this.#element = element
    }

    querySelector(selector: string): ManglerElement | null {
        const result = this.#element.querySelector(selector)
        return result && new ManglerElement(result, this.domish)
    }

    querySelectorAll(selector: string): ManglerElement[] {
        return Array.from(this.#element.querySelectorAll(selector)).map(e => new ManglerElement(e, this.domish))
    }

    setAttribute(name: string, value: StringResult) {
        const placeholder = this.domish.getPlaceholderForValue(value)
        this.#element.setAttribute(name, placeholder)
    }

    getAttribute(name: string) {
        return this.#element.getAttribute(name)
    }

    replace(node: NodeResult) {
        this.insertBefore(node)
        this.remove()
    }

    insertBefore(node: NodeResult): void {
        const content = this.domish.getPlaceholderForValue(node)
        this.#element.insertAdjacentHTML('beforebegin', trustedTypesPolicy.createHTML(content) as any)
    }

    insertAfter(node: NodeResult) {
        const content = this.domish.getPlaceholderForValue(node)
        this.#element.insertAdjacentHTML('afterend', trustedTypesPolicy.createHTML(content) as any)
    }

    appendChild(child: NodeResult) {
        const placeholder = this.domish.getPlaceholderForValue(child)
        this.#element.innerHTML = trustedTypesPolicy.createHTML(this.#element.innerHTML + placeholder)
    }
}

export function mangle(getHtml: () => TemplateResult, instance: any, mangler: (root: ManglerElement) => void) {
    const templated = getHtml.call(instance)
    const domish = new DOMIsh(templated)

    mangler.call(instance, domish.root)
    return domish.toTemplateResult()
}
