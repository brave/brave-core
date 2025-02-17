// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Note: This script runs as part of the build process, not at runtime, like our
// old Polymer overrides used to. It generates a new .html.ts file with our
// modifications to upstream's template.

import ts from 'typescript'
import { readFileSync, writeFileSync } from 'fs'
import path from 'path'
import {JSDOM} from 'jsdom'

interface HTMLTemplateTags {
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

export const load = (file: string) => {
    const input = getLiteralsFromFile(file)
    injectPlaceholders(input)
    return input
}

export const write = (file: string, result: HTMLTemplateTags) => {
    const text = replacePlaceholders(result)
    writeFileSync(file, text)
}

export const mangle = (template: HTMLTemplateTags, mangler: (element: DocumentFragment) => void) => {
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

export const findTemplate = (upstream: HTMLTemplateTags, predicate: (template: HTMLTemplateTags) => boolean) => {
    if (predicate(upstream)) {
        return upstream
    }

    for (const child of upstream.children) {
        const result = findTemplate(child, predicate)
        if (result) {
            return result
        }
    }
}
