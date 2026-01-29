/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import WebcompatReportModal from '../components/WebcompatReportModal'

import { useAppDispatch, useAppSelector } from '../hooks'
import { onSubmitReport, onClose } from '../slices/webcompatreporter.slice'

function WebcompatReportContainer() {
  const dispatch = useAppDispatch()
  const reporterState = useAppSelector((state) => state.reporterState)

  const handleSubmitReport = React.useCallback(
    (
      category: string,
      details: string,
      contact: string,
      attachScreenshot: boolean
    ) => {
      dispatch(onSubmitReport(category, details, contact, attachScreenshot))
    },
    [dispatch]
  )

  const handleClose = React.useCallback(() => {
    dispatch(onClose())
  }, [dispatch])

  if (!reporterState) {
    return null
  }

  return (
    <WebcompatReportModal
      siteUrl={reporterState.dialogArgs.url}
      contactInfo={reporterState.dialogArgs.contactInfo}
      contactInfoSaveFlag={reporterState.dialogArgs.contactInfoSaveFlag}
      isErrorPage={reporterState.dialogArgs.isErrorPage}
      submitted={reporterState.submitted}
      onSubmitReport={handleSubmitReport}
      onClose={handleClose}
      components={reporterState.dialogArgs.components}
    />
  )
}

export default WebcompatReportContainer
