/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import assert from 'assert'
import React from 'react'
import ReactTestUtils from 'react-dom/test-utils'
import BraveShieldsHeader from '../../../../app/components/braveShields/braveShieldsHeader'

function setup () {
  const props = {
    hostname: 'brave.com'
  }
  const renderer = ReactTestUtils.renderIntoDocument(<BraveShieldsHeader {...props} />)
  const result = renderer.render()
  return { props, result, renderer }
}

// TODO: @cezaraugusto Implement Enzyme
describe('BraveShieldsHeader component', () => {
  it('should render correctly', () => {
    const { result } = setup()
    assert.equal(result.props.id, 'braveShieldsHeader')
  })
})
