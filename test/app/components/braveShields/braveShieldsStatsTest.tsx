/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import { renderIntoDocument } from 'react-dom/test-utils'
import BraveShieldsStats from '../../../../app/components/braveShields/braveShieldsStats'
import { Props as UIProps } from 'brave-ui'
import { Props } from '../../../../app/components/braveShields/braveShieldsStats';

function setup () {
  const props: Props = {
    braveShields: 'allow',
    adsBlocked: 1,
    trackersBlocked: 2,
    httpsRedirected: 3,
    javascriptBlocked: 4,
    fingerprintingBlocked: 5
  }
  const renderer = renderIntoDocument(<BraveShieldsStats {...props} />) as React.Component<BraveShieldsStats>
  const result = renderer.render() as React.ReactElement<UIProps.Grid>
  return { props, result, renderer }
}

// TODO: @cezaraugusto Implement Enzyme
describe('BraveShieldsStats component', () => {
  it('should render correctly', () => {
    let { result } = setup()
    assert.equal(result.props.id, 'braveShieldsStats')
  })
})
