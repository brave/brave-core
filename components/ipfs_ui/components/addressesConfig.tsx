/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Section, Title } from '../style'

interface Props {
  addressesConfig: IPFS.AddressesConfig
}

export class AddressesConfig extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    return (
      <Section>
        <Title>
          <span i18n-content='addressesConfigTitle'/>
        </Title>
        <div>
          <span i18n-content='api'/>: {this.props.addressesConfig.api}
        </div>
        <div>
          <span i18n-content='gateway'/>: {this.props.addressesConfig.gateway}
        </div>
        <div>
          <span i18n-content='swarm'/>: {this.props.addressesConfig.swarm.toString()}
        </div>
      </Section>
    )
  }
}
