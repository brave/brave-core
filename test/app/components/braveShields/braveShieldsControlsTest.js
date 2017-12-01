/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import assert from 'assert'
import React from 'react'
import ReactTestUtils from 'react-dom/test-utils'
import BraveShieldsControls from '../../../../app/components/braveShields/braveShieldsControls'

function setup () {
  const props = {}
  const renderer = ReactTestUtils.renderIntoDocument(<BraveShieldsControls {...props} />)
  const result = renderer.render()
  return { props, result, renderer }
}

// TODO: @cezaraugusto Implement Enzyme
describe('BraveShieldsControls component', () => {
  it('should render correctly', () => {
    const { result } = setup()
    assert.equal(result.props.id, 'braveShieldsControls')
  })
})
