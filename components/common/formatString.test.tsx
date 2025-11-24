// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as assert from 'assert'
import * as React from 'react'
import { formatString } from './formatString'

// Note: We need to unmock the locale module here because we want to test it!
jest.unmock('$web-common/locale')

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

  it('should allow providing an array of replacements', () => {
    assert.equal(formatString('$1 say $2', ['I', () => 'Kiora' + '!']),
      'I say Kiora!')
  })

  it('should allow replacing a range', () => {
    assert.equal(formatString('$1 in $2nz/$2 say $3', {
      $1: 'People',
      $2: 'NZ',
      $3: 'Kiora'
    }), 'People in NZ say Kiora')
  })

  it('should allow replacing a range using an array of replacements', () => {
    assert.equal(formatString('$1 in $2nz/$2 say $3', ['People', 'NZ', 'Kiora']),
      'People in NZ say Kiora')
  })

  it('should allow replacing with a range and a function', () => {
    assert.equal(formatString('$1 in $2nz/$2 say $3', {
      $1: 'People',
      $2: (content) => content.toUpperCase(),
      $3: 'Kiora'
    }), 'People in NZ say Kiora')
  })

  it('should fail if a replacement does not exist', () => {
    assert.throws(() => formatString('$1 in $2nz/$2 say $3', {
      $1: 'People',
      $2: (content) => content.toUpperCase(),
      $3: 'Kiora',
      $4: 'MISSING!'
    }))
  })

  it('should fail if a replacement does not exist using an array of replacements', () => {
    assert.throws(() => formatString('$1 in $2nz/$2 say $3',
      ['People', 'NZ', 'Kiora', 'MISSING!']))
  })

  it('should not fail if a replacement does not exist if noErrorOnMissingReplacement is true', () => {
    expect(formatString('$1 in $2nz/$2 say $3', {
      $1: 'People',
      $2: (content) => content.toUpperCase(),
      $3: 'Kiora',
      $4: 'MISSING!'
    }, {
      noErrorOnMissingReplacement: true
    })).toBe('People in NZ say Kiora')
  })

  it('should not fail if a replacement does not exist if noErrorOnMissingReplacement is true using an array of replacements', () => {
    expect(formatString('$1 in $2nz/$2 say $3',
      ['People', 'NZ', 'Kiora', 'MISSING!'], {
        noErrorOnMissingReplacement: true
      })).toBe('People in NZ say Kiora')
  })

  it('should not change text with no replacements', () => {
    assert.equal(formatString('Hello world', {}), 'Hello world')
  })

  it('should not change text with no replacements using an array of replacements', () => {
    assert.equal(formatString('Hello world', []), 'Hello world')
  })

  it('should not change text with placeholders and no replacements using an array of replacements', () => {
    assert.equal(formatString('Hello $1', []), 'Hello $1')
  })

  it('should accept React nodes as replacements', () => {
    assert.deepEqual(formatString('Hello $1', {
      $1: <b>world</b>
    }), <>Hello <b>world</b></>)
  })

  it('should accept React nodes as array replacements', () => {
    assert.deepEqual(formatString('Hello $1', [<b>world</b>]),
      <>Hello <b>world</b></>)
  })

  it('should be able to return React nodes and take content as an argument', () => {
    assert.deepEqual(formatString('Hello $1world/$1', {
      $1: (content) => <b>{content}</b>
    }), <>Hello <b>world</b></>)
  })

  it('should be possible to mix text and React nodes', () => {
    assert.deepEqual(formatString('$1Shields are UP/$1 for $2', {
      $1: (content) => <span>{content}</span>,
      $2: 'brave.com'
    }), <><span>Shields are UP</span> for {"brave.com"}</>)
  })

  it('should be possible to only replace some keys', () => {
    assert.deepEqual(formatString('$1Shields are UP/$1 for $2', {
      $1: (content) => <span>{content}</span>,
    }), <><span>Shields are UP</span> for $2</>)
  })

  it('should be possible to only replace some keys out of order', () => {
    assert.deepEqual(formatString('$1Shields are UP/$1 for $2', {
      $2: "brave.com",
    }), "$1Shields are UP/$1 for brave.com")
  })

  it('one of everything', () => {
    assert.deepEqual(formatString('$1Shields are UP/$1 for $2 (count: $3) $4nz/$4', {
      $1: (content) => <span>{content}</span>,
      $2: "brave.com",
      $3: <span>over 9000</span>,
      $4: code => code.toUpperCase()
    }), <>
      <span>Shields are UP</span> for {"brave.com"} (count: <span>over 9000</span>) {"NZ"}
    </>)
  })

  it('should be possible to have a placeholder in a tag', () => {
    assert.deepEqual(formatString('Hello $1and goodbye $2/$1', {
      $1: (content) => <span>{content}</span>,
      $2: <b>world</b>
    }), <>Hello <span><>and goodbye <b>world</b></></span></>)
  })

  it('should not break for Czech shields', () => {
    assert.deepEqual(formatString('$1Štíty pro $2 jsou zapnuty/$1', {
      $1: content => <b>{content}</b>,
      $2: 'brave.com'
    }), <><b>Štíty pro brave.com jsou zapnuty</b></>)

    assert.deepEqual(formatString('$1Štíty pro $2 jsou vypnuty/$1', {
      $1: content => <b>{content}</b>,
      $2: 'brave.com'
    }), <><b>Štíty pro brave.com jsou vypnuty</b></>)
  })

  it('should be possible to nest tags', () => {
    assert.deepEqual(formatString('Hello $1and $3goodbye $2/$3/$1', {
      $1: (content) => <span>{content}</span>,
      $3: content => <i>{content}</i>,
      $2: <b>world</b>
    }), <>Hello <span><>and <i><>goodbye <b>world</b></></i></></span></>)
  })

  it('should be possible to replace multiple instances of the same key', () => {
    assert.deepEqual(formatString('Hello from $1 to $1', {
      $1: 'me'
    }), 'Hello from me to me')
  })

  it('should be possible to replace multiple instances of the same key with an array of replacements', () => {
    assert.deepEqual(formatString('Hello from $1 to $1', ['me']),
      'Hello from me to me')
  })

  it('should be possible to replace multiple instances of the same key with tags and nesting', () => {
    assert.deepEqual(formatString('Hello from $2$1/$2 to $1. $2bob/$2 says hi too', {
      $1: 'me',
      $2: (content) => <span>{content}</span>
    }), <>Hello from <span>me</span>{" to "}{"me"}. <span>bob</span> says hi too</>)
  })
})

