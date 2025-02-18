// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Note: This script runs as part of the build process, not at runtime, like our
// old Polymer overrides used to. It generates a new .html.ts file with our
// modifications to upstream's template.

// Note: This file is stateful, and is meant to be executed once per mangled
// file. It should be run via the command line.

import ts from 'typescript'
import { readFileSync, writeFileSync } from 'fs'
import path from 'path'
import { JSDOM } from 'jsdom'
import commander from 'commander'

export interface HTMLTemplateTags {
    id: number,
    text: string,
    children: HTMLTemplateTags[]
}

let nextId = 1

const loadRaw = (filepath: string) => {
    const text = readFileSync(filepath, 'utf-8')
    const tsFile = ts.createSourceFile(path.basename(filepath), text, ts.ScriptTarget.Latest)

    return tsFile
}

const getTemplateLiterals = (source: ts.SourceFile, node: ts.Node, templateTags: HTMLTemplateTags) => {
    if (ts.isTaggedTemplateExpression(node) && node.tag.getText(source) === 'html') {
        let text = node.template.getText(source)
        text = text.substring(1, text.length - 1)
        const tag: HTMLTemplateTags = {
            children: [],
            text,
            id: nextId++
        }

        getTemplateLiterals(source, node.template, tag)

        templateTags.children.push(tag)
        return
    }

    node.forEachChild(n => {
        getTemplateLiterals(source, n, templateTags)
    })
}

const getLiteralsFromFile = (filepath: string) => {
    const file = loadRaw(filepath)
    const root: HTMLTemplateTags = { text: file.getText(), children: [], id: nextId++ }
    getTemplateLiterals(file, file, root)

    if (root.children.length === 0) {
        throw new Error(`No templates found in ${filepath}`)
    }

    return root
}

const injectPlaceholders = (template: HTMLTemplateTags) => {
    for (const child of template.children) {
        const original = child.text
        child.text = injectPlaceholders(child)
        template.text = template.text.replace(original, `$$$$lit_mangler_${child.id}$$$$`)
    }

    return template.text
}

const replacePlaceholders = (template: HTMLTemplateTags) => {
    for (const child of template.children) {
        child.text = replacePlaceholders(child)
        template.text = template.text.replaceAll(`$$lit_mangler_${child.id}$$`, child.text)
    }

    return template.text
}

let result: HTMLTemplateTags | undefined = undefined

const load = (file: string) => {
    if (result !== undefined) {
        throw new Error('This should only be called once per file')
    }

    result = getLiteralsFromFile(file)
    injectPlaceholders(result)
}

const write = (file: string) => {
    if (!result) {
        throw new Error('This should only be called after load')
    }

    const text = replacePlaceholders(result)
    writeFileSync(file, text)
}

/**
 * Mangles a given html template using the given mangler function.
 *
 * Example usage:
 * mangle((element) => element.textContent = "foo", t => t.text.includes('allow-incognito'))
 *
 * @param mangler The function to use to mangle the template.
 * @param getTemplate The template to mangle, or a predicate to find a matching template. If undefined, the first html template will be used.
 */
export const mangle = (mangler: (element: DocumentFragment) => void, getTemplate?: HTMLTemplateTags | ((template: HTMLTemplateTags) => boolean)) => {
    if (!result) {
        throw new Error('This should only be called after load!')
    }

    let template: HTMLTemplateTags | undefined = undefined
    if (!getTemplate) {
        template = findTemplate(() => true)
    } else if (typeof getTemplate === 'function') {
        template = findTemplate(getTemplate)
    } else {
        template = getTemplate
    }

    // If we don't have a template throw an error - mangling isn't going to work.
    if (!template) {
        throw new Error(`Failed to find template matching ${getTemplate}. If this was working before upstream may have changed.`)
    }

    const world = new JSDOM()
    const document = world.window.document
    const element = document.createElement('template')
    element.innerHTML = template.text
        // Note: we ensure that all attributes are quoted so the template can be
        // parsed as HTML.
        // This won't work for attributes with braces in them, but it'll
        // drastically break the HTML output from upstrean, so it should be
        // pretty obvious that something went wrong.
        .replaceAll(/(\w+)(\s+)?=(\s+)?(\$\{.*?\})(\s|>)/gi, (...args) => {
            return `${args[1]}="${args[4].replaceAll('"', '&quot;')}"${args[5]}`
        })

    mangler(element.content)

    template.text = element.innerHTML
        .replaceAll('&gt;', '>')
        .replaceAll('&lt;', '<')
        .replaceAll('&amp;', '&')
        .replaceAll('&quot;', '"')
}

// Note: This doesn't check the template param against the predicate, only its children.
export const findTemplate = (predicate: (template: HTMLTemplateTags) => boolean, template?: HTMLTemplateTags): HTMLTemplateTags | undefined => {
    if (!result) {
        throw new Error("This should only be called after load!")
    }

    for (const child of (template ?? result).children) {
        if (predicate(child)) {
            return child
        }

        const result = findTemplate(predicate, child)
        if (result) {
            return result
        }
    }
}

// Utils to make testing easier
export const utilsForTest = {
    findTemplate,
    mangle,
    injectPlaceholders,
    replacePlaceholders,
    getTemplateLiterals,
    setResult: (r: HTMLTemplateTags) => {
        result = r
    },
    resetTemplateId: () => nextId = 1
}

if (process.argv.some(a => a.includes('lit_mangler'))) {
    commander
        .command('mangle')
        .option('-m, --mangler <file>', 'The file with containing the mangler instructions')
        .option('-i, --input <file>', 'The file to mangle')
        .option('-o, --output <file>', 'Where to output the mangled file')
        .action(async ({ mangler, input, output }: { mangler: string, input: string, output: string }) => {
            load(input)
            await import(mangler)
            write(output)
        })

    commander.parse(process.argv)
}
