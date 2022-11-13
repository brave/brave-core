/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../common/locale'

import { PaddedButton, BorderlessButton, Section, SideBySideButtons, Title, Error } from '../style'

interface Props {
  daemonStatus: IPFS.DaemonStatus
  onLaunch: () => void
  onShutdown: () => void
  onRestart: () => void
  onOpenNodeWebUI: () => void
  addressesConfig: IPFS.AddressesConfig
}

export class DaemonStatus extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    return (
      <Section>
        <Title>
          {getLocale('daemonStatusTitle')}
        </Title>
        {!this.props.daemonStatus.error.length && (
        <div>
          {(this.props.daemonStatus.launched && !this.props.daemonStatus.error) ? getLocale('launched') : getLocale('notLaunched')}
        </div>)}
        {this.props.daemonStatus.error.length > 0 && (
        <div
          style={Error}
        >
          {this.props.daemonStatus.error}
        </div>)}
        <SideBySideButtons>
          {(!this.props.daemonStatus.launched && !this.props.daemonStatus.restarting) && (<PaddedButton
            text={getLocale('launch')}
            size={'small'}
            onClick={this.props.onLaunch}
          />)}
          {this.props.daemonStatus.launched && (<PaddedButton
            text={getLocale('shutdown')}
            size={'small'}
            onClick={this.props.onShutdown}
          />)}
          {(this.props.daemonStatus.launched || this.props.daemonStatus.restarting) && (<PaddedButton
            text={getLocale('restart')}
            size={'small'}
            onClick={this.props.onRestart}
          />
          )}
          {this.props.addressesConfig.api && !this.props.daemonStatus.restarting && (
          <BorderlessButton
            text={getLocale('openWebUI')}
            size={'small'}
            onClick={this.props.onOpenNodeWebUI}
          />
          )}
        </SideBySideButtons>
      </Section>
    )
  }
}
