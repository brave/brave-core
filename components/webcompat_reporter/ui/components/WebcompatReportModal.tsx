/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components group
import ReportView from './ReportView'
import ConfirmationView from './ConfirmationView'
import { RectangularCard } from './basic'

interface Props {
  siteUrl: string
  contactInfo: string
  isErrorPage: boolean
  submitted: boolean
  onSubmitReport: () => void
  onClose: () => void
}

export default class WebcompatReportModal extends React.PureComponent<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    const {
      siteUrl,
      contactInfo,
      submitted,
      onSubmitReport,
      isErrorPage,
      onClose
    } = this.props

    const parsedUrl = new URL(siteUrl)
    const isHttpPage = parsedUrl.protocol === 'http:' || parsedUrl.protocol === 'https:'
    const isLocalPage = parsedUrl.hostname === '127.0.0.1' || parsedUrl.hostname === '::1' ||
      parsedUrl.hostname === 'localhost' || parsedUrl.hostname.endsWith('.local')

    return (
      <div onContextMenu={(e: React.MouseEvent<HTMLDivElement>) => e.preventDefault()}>
        <RectangularCard>
          {submitted ? (
            <ConfirmationView/>
          ) : (
            <ReportView
              siteUrl={siteUrl}
              contactInfo={contactInfo}
              isErrorPage={isErrorPage}
              isHttpPage={isHttpPage}
              isLocalPage={isLocalPage}
              onSubmitReport={onSubmitReport}
              onClose={onClose}
            />
          )}
        </RectangularCard>
      </div>
    )
  }
}
