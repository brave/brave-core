/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../common/locale'

import { Section, Title } from '../style'

interface Props {
  repoStats: IPFS.RepoStats
}

export class RepoStats extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    return (
      <Section>
        <Title>
          {getLocale('repoStatsTitle')}
        </Title>
        <div>
          {getLocale('objects')}: {this.props.repoStats.objects}
        </div>
        <div>
          {getLocale('size')}: {this.props.repoStats.size}
        </div>
        <div>
          {getLocale('storage')}: {this.props.repoStats.storage}
        </div>
        <div>
          {getLocale('path')}: {this.props.repoStats.path.toString()}
        </div>
        <div>
          {getLocale('version')}: {this.props.repoStats.version.toString()}
        </div>
      </Section>
    )
  }
}
