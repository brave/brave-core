/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import { renderIntoDocument } from 'react-dom/test-utils'
import BraveShieldsFooter from '../../../../app/components/braveShields/braveShieldsFooter'
import { GridProps } from 'brave-ui/gridSystem'

function setup () {
  const props = {}
  const renderer = renderIntoDocument(<BraveShieldsFooter {...props} />) as React.Component<BraveShieldsFooter>
  const result = renderer.render() as React.ReactElement<GridProps>
  return { props, result, renderer }
}

// TODO: @cezaraugusto Implement Enzyme
describe('BraveShieldsFooter component', () => {
  it('should render correctly', () => {
    const { result } = setup()
    assert.equal(result.props.id, 'braveShieldsFooter')
  })
})
