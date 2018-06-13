/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import BraveShieldsStats from '../../../../app/components/braveShields/braveShieldsStats'
import { BraveShieldsStatsProps } from '../../../../app/components/braveShields/braveShieldsStats';
import { shallow } from 'enzyme'

const fakeProps: BraveShieldsStatsProps = {
  braveShields: 'allow',
  adsBlocked: 1,
  trackersBlocked: 2,
  httpsRedirected: 3,
  javascriptBlocked: 4,
  fingerprintingBlocked: 5
}

describe('BraveShieldsStats component', () => {
  const baseComponent = (props: BraveShieldsStatsProps) =>
    <BraveShieldsStats {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#braveShieldsStats').length === 1
    assert.equal(assertion, true)
  })

  // TODO: add more tests after #202
})
