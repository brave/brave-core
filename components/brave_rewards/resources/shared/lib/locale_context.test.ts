/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { formatMessage } from './locale_context'

describe('locale_context', () => {
  describe('formatMesage', () => {
    it('fills placeholders with array values', () => {
      expect(formatMessage('a $1 b $3 c $2', [1, 2, 3]))
        .toStrictEqual(['a ', 1, ' b ', 3, ' c ', 2])
    })

    it('fills placeholders with a placeholders object', () => {
      expect(formatMessage('a $1 b $3 c $2', {
        placeholders: {
          $1: 1,
          $2: 2,
          $3: 3
        }
      })).toStrictEqual(['a ', 1, ' b ', 3, ' c ', 2])
    })

    it('fills repeated placeholders', () => {
      expect(formatMessage('-$1-$1-', ['x']).join('')).toBe('-x-x-')
    })

    it('replaces tags', () => {
      expect(formatMessage('a $3x$4 b $1y$2 c', {
        tags: {
          $1: (content) => `-${content}-`,
          $3: (content) => `=${content}=`
        }
      })).toStrictEqual(['a ', '', '=x=', '', ' b ', '', '-y-', '', ' c'])
    })

    it('fills placeholders and replaces tags', () => {
      expect(formatMessage('a $1x$2 $3', {
        placeholders: {
          $3: 3
        },
        tags: {
          $1: (content) => `-${content}-`
        }
      })).toStrictEqual(['a ', '', '-x-', '', ' ', 3])
    })

    it('replaces $$ with $', () => {
      expect(formatMessage('$$', [])).toStrictEqual(['$'])
    })

    it('replaces tags with nested placeholders', () => {
      expect(formatMessage('$1Hello, $2$3', {
        placeholders: {
          $2: 'world'
        },
        tags: {
          $1: {
            end: '$3',
            replace: (content) => ['===', content, '===']
          }
        }
      })).toStrictEqual([
        '',
        [
          '===',
          [
            'Hello, ',
            'world'
          ],
          '==='
        ],
        '',
        ''
      ])
    })
  })
})
