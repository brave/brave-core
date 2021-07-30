/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../common/locale'

import { Section, BlueLink } from '../style'

interface Props {
  addressesConfig: IPFS.AddressesConfig
  connectedPeers: IPFS.ConnectedPeers
  onOpenPeersWebUI: () => void
}

export class ConnectedPeers extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    return (
      <Section>
        <div>
          {getLocale('connectedPeersTitle')} {this.props.connectedPeers.peerCount}
          <span>&#8195;</span>
          {this.props.addressesConfig.api && (
            <a
              style={BlueLink}
              onClick={this.props.onOpenPeersWebUI}
            >
              {getLocale('peerDetailsLink')}
            </a>
          )}
        </div>
      </Section>
    )
  }
}
