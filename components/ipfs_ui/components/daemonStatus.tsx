/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../common/locale'

import { PaddedButton, Section, SideBySideButtons, Title } from '../style'

interface Props {
  daemonStatus: IPFS.DaemonStatus
  onLaunch: () => void
  onShutdown: () => void
}

export class DaemonStatus extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    return (
      <Section>
        <Title>
          <span i18n-content='daemonStatusTitle'/>
        </Title>
        <div>
          <span i18n-content='launched'/>: {this.props.daemonStatus.launched.toString()}
        </div>
        <SideBySideButtons>
          <PaddedButton
            text={getLocale('launch')}
            size={'small'}
            onClick={this.props.onLaunch}
          />
          <PaddedButton
            text={getLocale('shutdown')}
            size={'small'}
            onClick={this.props.onShutdown}
          />
        </SideBySideButtons>
      </Section>
    )
  }
}
