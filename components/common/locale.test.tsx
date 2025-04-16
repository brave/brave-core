// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as assert from 'assert'
import * as React from 'react'
import { formatString } from './locale'

describe('formatString', () => {
  it('should format a string with replacements', () => {
    assert.equal(formatString('Hello $1', {
      $1: 'world'
    }), 'Hello world')
  })

  it('should allow multiple replacements', () => {
    assert.equal(formatString('$1 say $2!', {
      $1: 'I',
      $2: 'Kiora'
    }), 'I say Kiora!')
  })

  it('should allow replacing with a function', () => {
    assert.equal(formatString('$1 say $2', {
      $1: 'I',
      $2: () => 'Kiora' + '!'
    }), 'I say Kiora!')
  })

  it('should allow replacing a range', () => {
    assert.equal(formatString('$1 in $2nz$2 say $3', {
      $1: 'People',
      $2: 'NZ',
      $3: 'Kiora'
    }), 'People in NZ say Kiora')
  })

  it('should allow replacing with a range and a function', () => {
    assert.equal(formatString('$1 in $2nz$2 say $3', {
      $1: 'People',
      $2: (content) => content.toUpperCase(),
      $3: 'Kiora'
    }), 'People in NZ say Kiora')
  })

  it('should fail if a replacement does not exist', () => {
    assert.throws(() => formatString('$1 in $2nz$2 say $3', {
      $1: 'People',
      $2: (content) => content.toUpperCase(),
      $3: 'Kiora',
      $4: 'MISSING!'
    }))
  })

  it('should fail when a range overlaps with another range', () => {
    assert.throws(() => formatString('$1 in $2nz$1 say $2', {
      $1: 'People',
      $2: (content) => content.toUpperCase(),
      $3: 'Kiora'
    }))
  })

  it('should not change text with no replacements', () => {
    assert.equal(formatString('Hello world', {}), 'Hello world')
  })

  it('should accept React nodes as replacements', () => {
    assert.deepEqual(formatString('Hello $1', {
      $1: <b>world</b>
    }), <>Hello <b>world</b></>)
  })

  it('should be able to return React nodes and take content as an argument', () => {
    assert.deepEqual(formatString('Hello $1world$1', {
      $1: (content) => <b>{content}</b>
    }), <>Hello <b>world</b></>)
  })

  it('should be possible to mix text and React nodes', () => {
    assert.deepEqual(formatString('$1Shields are UP$1 for $2', {
      $1: (content) => <span>{content}</span>,
      $2: 'brave.com'
    }), <><span>Shields are UP</span> for {"brave.com"}</>)
  })

  it('should be possible to only replace some keys', () => {
    assert.deepEqual(formatString('$1Shields are UP$1 for $2', {
      $1: (content) => <span>{content}</span>,
    }), <><span>Shields are UP</span> for $2</>)
  })

  it('should be possible to only replace some keys out of order', () => {
    assert.deepEqual(formatString('$1Shields are UP$1 for $2', {
      $2: "brave.com",
    }), "$1Shields are UP$1 for brave.com")
  })

  it('one of everything', () => {
    assert.deepEqual(formatString('$1Shields are UP$1 for $2 (count: $3) $4nz$4', {
      $1: (content) => <span>{content}</span>,
      $2: "brave.com",
      $3: <span>over 9000</span>,
      $4: code => code.toUpperCase()
    }), <>
      <span>Shields are UP</span> for {"brave.com"} (count: <span>over 9000</span>) {"NZ"}
    </>)
  })
})

