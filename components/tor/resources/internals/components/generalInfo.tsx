/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../../../common/locale'

interface Props {
  state: TorInternals.State
}

export class GeneralInfo extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    return (
      <div>
        <div>
          {getLocale('torVersion') + ': '} {this.props.state.generalInfo.torVersion}
        </div>
        <div>
          {getLocale('torPid') + ': '} {this.props.state.generalInfo.torPid}
        </div>
        <div>
          {getLocale('torProxyURI') + ': '} {this.props.state.generalInfo.torProxyURI}
        </div>
        <div>
          {getLocale('torConnectionStatus') + ': '} {this.props.state.generalInfo.isTorConnected ? 'Connected' : 'Disconnected'}
        </div>
        <div>
          {getLocale('torInitProgress') + ': '} {this.props.state.generalInfo.torInitPercentage}
          {this.props.state.generalInfo.torInitPercentage ? '%' : ''}
        </div>
      </div>
    )
  }
}
