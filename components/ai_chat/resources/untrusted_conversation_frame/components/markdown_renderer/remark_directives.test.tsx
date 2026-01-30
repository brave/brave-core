// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  remarkDirectives,
  ALLOWED_DIRECTIVES,
  directiveComponents,
} from './remark_directives'
import type { Node } from 'unist'

test('remarkDirectives processes allowed directives', () => {
  const tree: Node = {
    type: 'root',
    children: [
      {
        type: 'leafDirective',
        name: 'search',
        attributes: { query: 'test query', id: '123' },
        children: [],
      },
    ],
  }

  const plugin = remarkDirectives()
  plugin(tree)

  const directive = (tree as any).children[0]
  expect(directive.data).toBeDefined()
  expect(directive.data.hName).toBe('search')
  expect(directive.data.hProperties).toEqual({ query: 'test query', id: '123' })
})

test('remarkDirectives skips disallowed directives', () => {
  const tree: Node = {
    type: 'root',
    children: [
      {
        type: 'leafDirective',
        name: 'malicious',
        attributes: { foo: 'bar' },
        children: [],
      },
    ],
  }

  const plugin = remarkDirectives()
  plugin(tree)

  const directive = (tree as any).children[0]
  // Disallowed directives should not have data set
  expect(directive.data).toBeUndefined()
})

test('remarkDirectives handles multiple directives', () => {
  const tree: Node = {
    type: 'root',
    children: [
      {
        type: 'leafDirective',
        name: 'search',
        attributes: { query: 'first' },
        children: [],
      },
      {
        type: 'leafDirective',
        name: 'unauthorized',
        attributes: { bad: 'data' },
        children: [],
      },
      {
        type: 'leafDirective',
        name: 'search',
        attributes: { query: 'second' },
        children: [],
      },
    ],
  }

  const plugin = remarkDirectives()
  plugin(tree)

  // First directive should be processed
  expect((tree as any).children[0].data).toBeDefined()
  expect((tree as any).children[0].data.hName).toBe('search')
  expect((tree as any).children[0].data.hProperties).toEqual({ query: 'first' })

  // Second directive should be skipped
  expect((tree as any).children[1].data).toBeUndefined()

  // Third directive should be processed
  expect((tree as any).children[2].data).toBeDefined()
  expect((tree as any).children[2].data.hName).toBe('search')
  expect((tree as any).children[2].data.hProperties).toEqual({
    query: 'second',
  })
})

test('remarkDirectives preserves existing node data', () => {
  const tree: Node = {
    type: 'root',
    children: [
      {
        type: 'leafDirective',
        name: 'search',
        attributes: { query: 'test' },
        data: { existingProperty: 'preserved' },
        children: [],
      },
    ],
  }

  const plugin = remarkDirectives()
  plugin(tree)

  const directive = (tree as any).children[0]
  expect(directive.data.hName).toBe('search')
  expect(directive.data.hProperties).toEqual({ query: 'test' })
  expect(directive.data.existingProperty).toBe('preserved')
})

test('ALLOWED_DIRECTIVES contains expected directives', () => {
  expect(ALLOWED_DIRECTIVES).toContain('search')
  expect(ALLOWED_DIRECTIVES.length).toBeGreaterThan(0)
})

test('directiveComponents has components for all allowed directives', () => {
  ALLOWED_DIRECTIVES.forEach((directive) => {
    expect(directiveComponents[directive]).toBeDefined()
    expect(typeof directiveComponents[directive]).toBe('function')
  })
})

test('search directive component returns null', () => {
  const result = directiveComponents.search({})
  expect(result).toBeNull()
})
