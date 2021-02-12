/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../common/locale'

import { PaddedButton, Section, Title } from '../style'

interface Props {
  daemonStatus: IPFS.DaemonStatus
  onLaunch: () => void
}

export class UninstalledView extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    return (
      <Section>
        <Title>
          {getLocale('daemonStatusTitle')}
        </Title>
        <div>
          {getLocale('not_installed')}
        </div>
        {!this.props.daemonStatus.installed && (
          <PaddedButton
            text={getLocale('install_and_launch')}
            size={'small'}
            onClick={this.props.onLaunch}
          />
        )}
      </Section>
    )
  }
}
