// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import { MockContext } from '../../state/mock_context'
import FeedbackForm from './'
import styles from '../../stories/style.module.scss'

export const _FeedbackForm = {
  render: () => {
    return (
      <MockContext>
        <div className={styles.container}>
          <FeedbackForm />
        </div>
      </MockContext>
    )
  },
}

export default {
  title: 'AI Chat/FeedbackForm',
  component: FeedbackForm,
} as Meta<typeof FeedbackForm>
