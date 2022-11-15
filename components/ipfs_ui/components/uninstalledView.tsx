/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../common/locale'

import { PaddedButton, Section, Title, Error } from '../style'

interface Props {
  daemonStatus: IPFS.DaemonStatus
  installationProgress: IPFS.InstallationProgress
  onInstall: () => void
}

export class UninstalledView extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  progress () {
    if (!this.props.daemonStatus.installing ||
        this.props.installationProgress.downloaded_bytes === -1 ||
        this.props.installationProgress.total_bytes === -1) {
      return ''
    }
    const bytes = this.props.installationProgress.downloaded_bytes
    const total = this.props.installationProgress.total_bytes
    if (!total) {
      return ''
    }
    const percentages = 100 * bytes / total
    return <span>{percentages.toFixed(2)}% </span>
  }

  disableInstallButton () {
    return this.props.daemonStatus.installing &&
           !this.props.daemonStatus.error.length
  }

  render () {
    return (
      <Section>
        <Title>
          {getLocale('daemonStatusTitle')}
        </Title>
        <div>
          {this.props.daemonStatus.installing ? getLocale('installing') : getLocale('notInstalled')}
          {this.progress()}
        </div>
        {this.props.daemonStatus.error.length > 0 && (
        <div
          style={Error}
        >
          {this.props.daemonStatus.error}
        </div>)}
        {!this.props.daemonStatus.installed && (
          <PaddedButton
            text={getLocale('installAndLaunch')}
            size={'small'}
            disabled={this.disableInstallButton()}
            onClick={this.props.onInstall}
          />
        )}
      </Section>
    )
  }
}
