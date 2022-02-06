
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
          {getLocale('id')}: {this.props.nodeInfo.id}
        </div>
        <div>
          {getLocale('version')}: {this.props.nodeInfo.version}
        </div>
        {this.props.nodeInfo.component_version &&
        <div>
          {getLocale('componentVersion')}: {this.props.nodeInfo.component_version}
        </div>}
      </Section>
    )
  }
}
