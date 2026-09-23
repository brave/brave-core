// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import Attachments from './'
import { MockContext } from '../../state/mock_context'
import styles from '../../../page/stories/style.module.scss'

export default {
  title: 'AI Chat/Attachments Panel',
  component: Attachments,
} as Meta

export const _AttachmentsPanel = {
  render: () => {
    return (
      <div className={styles.container}>
        <MockContext>
          <Attachments />
        </MockContext>
      </div>
    )
  },
}
