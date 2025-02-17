// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Note: This script runs as part of the build process, not at runtime, like our
// old Polymer overrides used to. It generates a new .html.ts file with our
// modifications to upstream's template.

import { JSDOM } from 'jsdom'

import { readFileSync, writeFileSync } from 'fs'

// A specific instance of the html tagged template literal.
interface Template {
    tag?: string,
    rawText: string,
    id: number,
    startIndex: number,
    endIndex: number,

    subtemplates: Template[]
}

// The result of parsing a .html.ts file.
interface Result {
    text: string,
    templates: Template[]
}

// Gets the line and column of the given index in the text for printing errors.
const getLineColumn = (text: string, index: number) => {
    const lines = text.substring(0, index).split('\n')
    return [
        lines.length,
        lines.at(-1)!.length
    ]
}

// We need a unique ID for each template in the file, so we can put things back
// together after we've mangled them.
let nextId = 1

// Note: This method is super niaive and doesn't attempt to handle nesting non-html
// tagged template literals. For the .html.ts file in Chromium this seems like
// its going to be good enough for most cases.
function* readTags(filepath: string, text: string): Iterable<Template> {
    let i = 0
    const stack: Pick<Template, 'id' | 'startIndex' | 'tag' | 'subtemplates'>[] = []

    const consumeUntilMatch = () => {
        i = text.indexOf(text[i], i + 1)
    }

    for (; i < text.length; ++i) {
        const isInTemplate = stack.length !== 0
        const char = text.at(i)

        // If we're not in a template and we did a string, read till the end
        if (!isInTemplate && (char == `"` || char === `'`)) {
            consumeUntilMatch()
            continue
        }

        // We're only really interested in backticks
        if (char !== '`') continue

        // This backtick is a literal, so ignore it
        if (text[i - 1] === '\\') continue

        // This is the start of an HTML template literal
        if (text.substring(i - 4, i) === 'html') {
            stack.push({
                tag: 'html',
                id: nextId++,
                startIndex: i,
                subtemplates: []
            })
            continue
        }

        // This must be an end tag!
        const templateStart = stack.pop()!
        if (!templateStart) {
            throw new Error(`Encountered close tag without open at ${filepath}:${getLineColumn(text, i).join(':')}`)
        }

        const template: Template = {
            ...templateStart,
            endIndex: i,
            rawText: text.substring(templateStart.startIndex + 1, i)
        }
        if (stack.length) {
            stack.at(-1)?.subtemplates.push(template)
            continue;
        }
        yield template
    }
}

const setTemplatePlaceholders = (text: string, templates: Template[]) => {
    for (const template of templates) {
        const original = template.rawText
        template.rawText = setTemplatePlaceholders(template.rawText, template.subtemplates)
        text = text.replace(original, `$$$$lit_mangler_${template.id}$$$$`)
    }
    return text
}

const replacePlaceholders = (text: string, templates: Template[]) => {
    for (const template of templates) {
        template.rawText = replacePlaceholders(template.rawText, template.subtemplates)
        text = text.replaceAll(`$$lit_mangler_${template.id}$$`, template.rawText)
    }

    return text
}

export const loadRaw = (path: string): Result => {
    const text = readFileSync(path, 'utf-8')
        .replaceAll(/(\w+)(\s+)?=(\s+)?(\$\{.*?\})(\s|>)/gi, "$1='$4'$5")

    const templates = [...readTags(path, text)]
    const modifiedText = setTemplatePlaceholders(text, templates)

    return {
        text: modifiedText,
        templates: templates
    }
}

export const write = (file: string, result: Result) => {
    const text = replacePlaceholders(result.text, result.templates)
    writeFileSync(file, text)
}

export const findTemplate = (from: Template[] | Result, predicate: (template: Template) => boolean) => {
    const templates = 'templates' in from ? from.templates : from

    // Search this level first
    for (const template of templates) {
        if (predicate(template)) {
            return template
        }
    }

    // Then search the subtemplates
    for (const template of templates) {
        const result = findTemplate(template.subtemplates, predicate)
        if (result) {
            return result
        }
    }
}

export const mangle = (template: Template, mangler: (element: DocumentFragment) => void) => {
    const world = new JSDOM()
    const document = world.window.document
    const element = document.createElement('template')
    element.innerHTML = template.rawText

    mangler(element.content)

    // We need to unescape the HTML - Lit will escape it for us when it creates
    // its templates.
    template.rawText = element.innerHTML
        .replaceAll('&gt;', '>')
        .replaceAll('&lt;', '<')
        .replaceAll('&amp;', '&')
        .replaceAll('&quot;', '"')
}
