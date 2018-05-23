/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import { renderIntoDocument } from 'react-dom/test-utils'
import BraveShieldsControls, { Props } from '../../../../app/components/braveShields/braveShieldsControls'
import { BlockOptions, BlockFPOptions, BlockCookiesOptions } from '../../../../app/types/other/blockTypes'
import * as actionTypes from '../../../../app/constants/shieldsPanelTypes'
import { GridProps } from 'brave-ui/gridSystem'

function setup () {
  const props: Props = {
    controlsOpen: false,
    braveShields: 'allow',
    httpUpgradableResources: 'allow',
    ads: 'allow',
    trackers: 'allow',
    javascript: 'block',
    fingerprinting: 'block',
    cookies: 'block',
    noScriptInfo: {},
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
    },
    blockCookies: (setting: BlockCookiesOptions) => {
      return {
        type: actionTypes.BLOCK_COOKIES,
        setting
      }
    },
    allowScriptOriginsOnce: (origins: string[]) => {
      return {
        type: actionTypes.ALLOW_SCRIPT_ORIGINS_ONCE,
        origins
      }
    },
    changeNoScriptSettings: (origin: string) => {
      return {
        type: actionTypes.CHANGE_NO_SCRIPT_SETTINGS,
        origin
      }
    }
  }

  const renderer = renderIntoDocument(<BraveShieldsControls {...props} />) as React.Component<BraveShieldsControls>
  const result = renderer.render() as React.ReactElement<GridProps>
  return { props, result, renderer }
}

// TODO: @cezaraugusto Implement Enzyme
describe('BraveShieldsControls component', () => {
  it('should render correctly', () => {
    const { result } = setup()
    assert.equal(result.props.id, 'braveShieldsControls')
  })
})
