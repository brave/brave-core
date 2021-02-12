/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../common/locale'

import { Section, Title, BlueLink } from '../style'

interface Props {
  addressesConfig: IPFS.AddressesConfig
  daemonStatus: IPFS.DaemonStatus
  onOpenNodeWebUI: () => void
}

export class AddressesConfig extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    return (
      <Section>
        <Title>
          {getLocale('addressesConfigTitle')}
        </Title>
        <div>
          {getLocale('api')}: {this.props.addressesConfig.api}
          <span>&#8195;</span>
          {this.props.addressesConfig.api && !this.props.daemonStatus.restarting && (
          <a
            style={BlueLink}
            onClick={this.props.onOpenNodeWebUI}
          >
            {getLocale('open_webui')}
          </a>)}
        </div>
        <div>
          {getLocale('gateway')}: {this.props.addressesConfig.gateway}
        </div>
        <div>
          {getLocale('swarm')}: {this.props.addressesConfig.swarm.toString()}
        </div>
      </Section>
    )
  }
}
