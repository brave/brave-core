/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import assert from 'assert'
import React from 'react'
import ReactTestUtils from 'react-dom/test-utils'
import BraveShieldsFooter from '../../../../app/components/braveShields/braveShieldsFooter'

function setup () {
  const props = {}
  const renderer = ReactTestUtils.renderIntoDocument(<BraveShieldsFooter {...props} />)
  const result = renderer.render()
  return { props, result, renderer }
}

describe('BraveShieldsFooter component', () => {
  it('should render correctly', () => {
    const { result } = setup()
    assert.equal(result.type, 'header')
    const h1 = result.props.children
    assert.equal(h1.type, 'h1')
    assert.equal(h1.props.children, 'Footer')
  })
})
