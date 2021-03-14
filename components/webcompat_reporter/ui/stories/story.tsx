/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import WebcompatReportModal from './modal/index'

export default {
  title: 'Webcompat Reporter'
}

export const BeforeSubmit = () => {
  const fakeOnSubmit = () => {
    /* noop */
  }
  const fakeOnClose = () => {
    /* noop */
  }
  return (
    <div style={{ width: '375px', margin: '0 auto' }}>
      <WebcompatReportModal
        siteUrl={'https://www.buzzfeed.com/'}
        submitted={false}
        onSubmitReport={fakeOnSubmit}
        onClose={fakeOnClose}
      />
    </div>
  )
}

BeforeSubmit.story = {
  name: 'Before submit'
}

export const SubmitConfirmation = () => {
  const fakeOnSubmit = () => {
    /* noop */
  }
  const fakeOnClose = () => {
    /* noop */
  }
  return (
    <div style={{ width: '375px', margin: '0 auto' }}>
      <WebcompatReportModal
        siteUrl={'https://www.buzzfeed.com/'}
        submitted={true}
        onSubmitReport={fakeOnSubmit}
        onClose={fakeOnClose}
      />
    </div>
  )
}

SubmitConfirmation.story = {
  name: 'Submit confirmation'
}
