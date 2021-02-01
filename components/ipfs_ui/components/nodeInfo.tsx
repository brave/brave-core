/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../common/locale'

import { Section, Title } from '../style'

interface Props {
  nodeInfo: IPFS.NodeInfo
}

export class NodeInfo extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    return (
      <Section>
        <Title>
          {getLocale('nodeInfoTitle')}
        </Title>
        <div>
          {getLocale('peerId')}: {this.props.nodeInfo.id.toString()}
        </div>
        <div>
          {getLocale('agentVersion')}: {this.props.nodeInfo.version.toString()}
        </div>
      </Section>
    )
  }
}
