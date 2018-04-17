/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import { renderIntoDocument } from 'react-dom/test-utils'
import BraveShieldsControls, { Props } from '../../../../app/components/braveShields/braveShieldsControls'
import { BlockOptions, BlockFPOptions } from '../../../../app/types/other/blockTypes'
import * as actionTypes from '../../../../app/constants/shieldsPanelTypes'
import { Props as UIProps } from 'brave-ui'

function setup () {
  const props: Props = {
    controlsOpen: false,
    braveShields: 'allow',
    httpUpgradableResources: 'allow',
    ads: 'allow',
    trackers: 'allow',
    javascript: 'block',
    fingerprinting: 'block',
    blockAdsTrackers: (setting: BlockOptions) => {
      return {
        type: actionTypes.BLOCK_ADS_TRACKERS,
        setting
      }
    },
    controlsToggled: (setting: boolean) => {
      return {
        type: actionTypes.CONTROLS_TOGGLED,
        setting
      }
    },
    httpsEverywhereToggled: () => {
      return {
        type: actionTypes.HTTPS_EVERYWHERE_TOGGLED
      }
    },
    javascriptToggled: () => {
      return {
        type: actionTypes.JAVASCRIPT_TOGGLED
      }
    },
    blockFingerprinting: (setting:BlockFPOptions) => {
      return {
        type: actionTypes.BLOCK_FINGERPRINTING,
        setting
      }
    }
  }

  const renderer = renderIntoDocument(<BraveShieldsControls {...props} />) as React.Component<BraveShieldsControls>
  const result = renderer.render() as React.ReactElement<UIProps.Grid>
  return { props, result, renderer }
}

// TODO: @cezaraugusto Implement Enzyme
describe('BraveShieldsControls component', () => {
  it('should render correctly', () => {
    const { result } = setup()
    assert.equal(result.props.id, 'braveShieldsControls')
  })
})
