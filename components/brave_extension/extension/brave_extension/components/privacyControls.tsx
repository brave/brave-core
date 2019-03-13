/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Group Components
import ScriptsControl from './controls/scriptsControl'
import CookiesControl from './controls/cookiesControl'
import DeviceRecognitionControl from './controls/deviceRecognitionControl'

// Types
import {
  ChangeNoScriptSettings,
  BlockJavaScript,
  ChangeAllNoScriptSettings,
  AllowScriptOriginsOnce,
  BlockCookies,
  BlockFingerprinting
} from '../types/actions/shieldsPanelActions'
import { BlockCookiesOptions, BlockJSOptions, BlockFPOptions } from '../types/other/blockTypes'
import { NoScriptInfo } from '../types/other/noScriptInfo'

interface CommonProps {
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  hostname: string
  favicon: string
}

interface JavaScriptProps {
  javascript: BlockJSOptions
  javascriptBlocked: number
  noScriptInfo: NoScriptInfo
  changeNoScriptSettings: ChangeNoScriptSettings
  blockJavaScript: BlockJavaScript
  changeAllNoScriptSettings: ChangeAllNoScriptSettings
  allowScriptOriginsOnce: AllowScriptOriginsOnce
}

interface CookiesProps {
  blockCookies: BlockCookies
  cookies: BlockCookiesOptions
}

interface FingerprintingProps {
  fingerprinting: BlockFPOptions
  fingerprintingBlocked: number
  fingerprintingBlockedResources: Array<string>
  blockFingerprinting: BlockFingerprinting
}

export type Props = CommonProps & JavaScriptProps & CookiesProps & FingerprintingProps

export default class PrivacyControls extends React.PureComponent<Props, {}> {
  get commonProps (): CommonProps {
    const { favicon, hostname, isBlockedListOpen, setBlockedListOpen } = this.props
    return { favicon, hostname, isBlockedListOpen, setBlockedListOpen }
  }

  render () {
    return (
      <>
        <ScriptsControl
          {...this.commonProps}
          javascript={this.props.javascript}
          javascriptBlocked={this.props.javascriptBlocked}
          noScriptInfo={this.props.noScriptInfo}
          changeNoScriptSettings={this.props.changeNoScriptSettings}
          blockJavaScript={this.props.blockJavaScript}
          changeAllNoScriptSettings={this.props.changeAllNoScriptSettings}
          allowScriptOriginsOnce={this.props.allowScriptOriginsOnce}
        />
        <CookiesControl
          isBlockedListOpen={this.props.isBlockedListOpen}
          cookies={this.props.cookies}
          blockCookies={this.props.blockCookies}
        />
        <DeviceRecognitionControl
          {...this.commonProps}
          fingerprintingBlocked={this.props.fingerprintingBlocked}
          fingerprinting={this.props.fingerprinting}
          fingerprintingBlockedResources={this.props.fingerprintingBlockedResources}
          blockFingerprinting={this.props.blockFingerprinting}
        />
      </>
    )
  }
}
