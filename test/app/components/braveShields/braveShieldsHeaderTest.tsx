/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import { renderIntoDocument } from 'react-dom/test-utils'
import BraveShieldsHeader, { Props } from '../../../../app/components/braveShields/braveShieldsHeader'
import { GridProps } from 'brave-ui/gridSystem'
import { BlockOptions } from '../../../../app/types/other/blockTypes';
import * as actionTypes from '../../../../app/constants/shieldsPanelTypes'

function setup () {
  const props: Props = {
    hostname: 'brave.com',
    shieldsToggled: (setting: BlockOptions) => {
      return {
        type: actionTypes.SHIELDS_TOGGLED,
        setting
      }
    },
    braveShields: 'allow'
  }
  const renderer = renderIntoDocument(<BraveShieldsHeader {...props} />) as React.Component<BraveShieldsHeader>
  const result= renderer.render() as React.ReactElement<GridProps>
  return { props, result, renderer }
}

// TODO: @cezaraugusto Implement Enzyme
describe('BraveShieldsHeader component', () => {
  it('should render correctly', () => {
    const { result } = setup()
    assert.equal(result.props.id, 'braveShieldsHeader')
  })
})
