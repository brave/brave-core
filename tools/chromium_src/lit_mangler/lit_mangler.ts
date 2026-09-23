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

export interface HTMLTemplateTags {
  id: number
  text: string
  children: HTMLTemplateTags[]
}

let nextId = 1
let isHTML = false

// Properties are camelCase but will be converted to lowercase by the HTML
// parser. We need to keep track of the proper casing so we don't break setting
// properties.
const propertyRegex = /\s\.(\w+)/gm
const propertyCases: Record<string, string> = {}

const loadRaw = (filepath: string) => {
  isHTML = filepath.endsWith('.html')

  const text = readFileSync(filepath, 'utf-8')
  cachePropertyCasesFromText(text)
  const tsFile = ts.createSourceFile(
    path.basename(filepath),
    text,
    ts.ScriptTarget.Latest,
  )

  return tsFile
}

/**
 * Recursively loads the HTML template literals from a file.
 *
 * @param source The source file to load the template literals from.
 * @param node The node to load the template literals from.
 * @param templateTags The template tags to load the template literals into.
 */
const getTemplateLiterals = (
  source: ts.SourceFile,
  node: ts.Node,
  templateTags: HTMLTemplateTags,
) => {
  if (
    ts.isTaggedTemplateExpression(node)
    && node.tag.getText(source) === 'html'
  ) {
    let text = node.template.getText(source)
    text = text.substring(1, text.length - 1)
    const tag: HTMLTemplateTags = {
      children: [],
      text,
      id: nextId++,
    }

    getTemplateLiterals(source, node.template, tag)

    templateTags.children.push(tag)
    return
  }

  node.forEachChild((n) => {
    getTemplateLiterals(source, n, templateTags)
  })
}

/**
 * Stores the property cases from the source text in |propertyCases|. They need
 * to be restored when we save the file.
 * @param text The source text of the file
 */
const cachePropertyCasesFromText = (text: string) => {
  const matches = text.matchAll(propertyRegex)
  for (const match of matches) {
    propertyCases[match[0].toLocaleLowerCase()] = match[0]
  }
}

/**
 * Restores the property cases from |propertyCases| in the source text.
 * @param text The source text of the file
 * @returns The source text with the property cases restored
 */
const restorePropertyCases = (text: string) => {
  for (const [key, value] of Object.entries(propertyCases)) {
    text = text.replaceAll(key, value)
  }
  return text
}

/**
 * Loads the HTML template literals from a file.
 *
 * @param filepath The file to load the template literals from.
 */
const getLiteralsFromFile = (filepath: string) => {
  const file = loadRaw(filepath)
  const root: HTMLTemplateTags = {
    text: file.getText(),
    children: [],
    id: nextId++,
  }
  getTemplateLiterals(file, file, root)

  if (root.children.length === 0) {
    throw new Error(`No templates found in ${filepath}`)
  }

  return root
}

/**
 * Injects placeholders into the template so we can later replace them with
 * the mangled templates.
 *
 * (i.e.)
 *
 * export function getHtml(this: Foo) {
 *   return html`<div>Hello</div>`
 * }
 *
 * goes to
 *
 * export function getHtml(this: Foo) {
 *   return html`$$lit_mangler_1$$`
 * }
 *
 * This means we parse the template as HTML and later reinject the template back
 * into the source code (with replacePlaceholders).
 *
 * Note: If a template contains another template, that template will also be
 * replaced with a placeholder. This means we can mangle each template without
 * worrying about where it exists in the source code.
 *
 * (i.e.)
 *
 * export function getHtml(this: Foo) {
 *   return html`<div>${this.isBold ? html`<b>Hi</b>` : 'Hi'}</div>`
 * }
 *
 * goes to
 *
 * export function getHtml(this: Foo) {
 *   return html`$$lit_mangler_1$$`
 * }
 *
 * and then $$lit_mangler_1$$ is:
 *
 * `<div>${this.isBold ? html`$$lit_mangler_2$$` : 'Hi'}</div>`
 *
 * and $$lit_mangler_2$$ is:
 *
 * `<b>Hi</b>`
 *
 * @param template The template to inject placeholders into.
 */
const injectPlaceholders = (template: HTMLTemplateTags) => {
  for (const child of template.children) {
    const original = child.text
    child.text = injectPlaceholders(child)
    template.text = template.text.replace(
      original,
      `$$$$lit_mangler_${child.id}$$$$`,
    )
  }

  return template.text
}

/**
 * Reassembles the source code by replacing template placeholders with the
 * possibly mangled templates.
 *
 * (i.e.)
 *
 * export function getHtml(this: Foo) {
 *   return html`$$lit_mangler_1$$`
 * }
 *
 * goes to
 *
 * export function getHtml(this: Foo) {
 *   return html`<div>Hello</div>`
 * }
 *
 * @param template The template to replace the placeholders in.
 */
const replacePlaceholders = (template: HTMLTemplateTags) => {
  for (const child of template.children) {
    child.text = replacePlaceholders(child)
    template.text = template.text.replaceAll(
      `$$lit_mangler_${child.id}$$`,
      child.text,
    )
  }

  return template.text
}

let result: HTMLTemplateTags | undefined

/**
 * Loads the template literals from a file.
 *
 * @param file The file to load the template literals from.
 */
export const load = (file: string) => {
  if (result !== undefined) {
    throw new Error('This should only be called once per file')
  }

  result = getLiteralsFromFile(file)
  injectPlaceholders(result)
}

/**
 * Writes the mangled template to a file.
 *
 * @param file The file to write the mangled template to.
 */
export const write = (file: string) => {
  if (!result) {
    throw new Error('This should only be called after load')
  }

  const text = restorePropertyCases(replacePlaceholders(result))
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
export const mangle = (
  mangler: (element: DocumentFragment) => void,
  getTemplate?: HTMLTemplateTags | ((template: HTMLTemplateTags) => boolean),
) => {
  if (!result) {
    throw new Error('This should only be called after load!')
  }

  let template: HTMLTemplateTags | undefined
  if (!getTemplate) {
    template = findTemplates(() => true).next().value
  } else if (typeof getTemplate === 'function') {
    template = findTemplates(getTemplate).next().value
  } else {
    template = getTemplate
  }

  // If we don't have a template throw an error - mangling isn't going to work.
  if (!template) {
    throw new Error(
      `Failed to find template matching ${getTemplate}. If this was working before upstream may have changed.`,
    )
  }

  const world = new JSDOM()
  const document = world.window.document
  const element = document.createElement('template')
  element.innerHTML = template.text
    // Note: we ensure that all attributes are quoted so the template can be
    // parsed as HTML.
    // This won't work for attributes with braces in them, but it'll
    // drastically break the HTML output from upstream, so it should be
    // pretty obvious that something went wrong.
    // This turns `foo=${bar}` into `foo="bar"`
    .replaceAll(/(\w+)(\s+)?=(\s+)?(\$\{.*?\})(\s|>)/gi, '$1="$4"$5')
    // Replace all quotes in interpolated strings with &quot; so we don't
    // break the HTML output.
    // This turns `foo="${bar ? "one" : "two"}"` into
    // `foo="${bar ? &quot;one&quot; : &quot;two&quot;}"`
    .replaceAll(/\$\{.*?\}/gis, (...args) => {
      return args[0]
        .replaceAll('&', '&amp;') // Note: &amp; needs to be first
        .replaceAll('"', '&quot;')
        .replaceAll('<', '&lt;')
        .replaceAll('>', '&gt;')
    })

  mangler(element.content)

  template.text = element.innerHTML.replaceAll(/\$\{.*?\}/gis, (...args) => {
    return args[0]
      .replaceAll('&gt;', '>')
      .replaceAll('&lt;', '<')
      .replaceAll('&quot;', '"')
      .replaceAll('&amp;', '&') // Note: &amp; needs to be last
  })
}

/**
 * Mangles all templates that match the given predicate.
 *
 * @param mangler The function to use to mangle the template.
 * @param matchTemplate The predicate to use to find the templates to mangle.
 */
export const mangleAll = (
  mangler: (element: DocumentFragment) => void,
  matchTemplate: (template: HTMLTemplateTags) => boolean,
) => {
  for (const template of findTemplates(matchTemplate)) {
    mangle(mangler, template)
  }
}

// Note: This doesn't check the template param against the predicate, only its children.
export function* findTemplates(
  predicate: (template: HTMLTemplateTags) => boolean,
  template?: HTMLTemplateTags,
): IterableIterator<HTMLTemplateTags> {
  if (!result) {
    throw new Error('This should only be called after load!')
  }

  // If we're mangling an HTML file, the root node is also a template (though
  // it isn't tagged)
  if (isHTML && !template && predicate(result)) {
    yield result
  }

  for (const child of (template ?? result).children) {
    if (predicate(child)) {
      yield child
    }

    yield* findTemplates(predicate, child)
  }
}

// Utils to make testing easier
export const utilsForTest = {
  findTemplates,
  mangle,
  mangleAll,
  injectPlaceholders,
  replacePlaceholders,
  getTemplateLiterals,
  setResult: (r: HTMLTemplateTags | undefined) => {
    result = r
  },
  getResult: () => result,
  resetTemplateId: () => (nextId = 1),
  load,
  cachePropertyCasesFromText,
  restorePropertyCases,
}
