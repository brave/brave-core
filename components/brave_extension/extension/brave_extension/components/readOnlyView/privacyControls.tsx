/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Group components
import ScriptsControl from './controls/scriptsControl'
import CookiesControl from './controls/cookiesControl'
import DeviceRecognitionControl from './controls/deviceRecognitionControl'

// Types
import { NoScriptInfo } from '../../types/other/noScriptInfo'
import { BlockJSOptions, BlockFPOptions } from '../../types/other/blockTypes'

interface JavaScriptProps {
  javascript: BlockJSOptions
  javascriptBlocked: number
  noScriptInfo: NoScriptInfo
}

interface FingerprintingProps {
  fingerprinting: BlockFPOptions
  fingerprintingBlocked: number
  fingerprintingBlockedResources: Array<string>
}

export type Props = JavaScriptProps & FingerprintingProps

export default class PrivacyControls extends React.PureComponent<Props, {}> {
  render () {
    return (
      <>
        <ScriptsControl
          javascript={this.props.javascript}
          javascriptBlocked={this.props.javascriptBlocked}
          noScriptInfo={this.props.noScriptInfo}
        />
        <CookiesControl />
        <DeviceRecognitionControl
          fingerprintingBlocked={this.props.fingerprintingBlocked}
          fingerprinting={this.props.fingerprinting}
          fingerprintingBlockedResources={this.props.fingerprintingBlockedResources}
        />
      </>
    )
  }
}
