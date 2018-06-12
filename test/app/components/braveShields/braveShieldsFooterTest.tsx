/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import BraveShieldsFooter from '../../../../app/components/braveShields/braveShieldsFooter'
import { shallow } from 'enzyme'

describe('BraveShieldsFooter component', () => {
  const baseComponent = () => <BraveShieldsFooter />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent())
    const assertion = wrapper.find('#braveShieldsFooter').length === 1
    assert.equal(assertion, true)
  })
})
