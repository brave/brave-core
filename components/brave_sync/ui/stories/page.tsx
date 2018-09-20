/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'

// Components
import SyncPage from './page/index'

storiesOf('Feature Components/Sync/Page', module)
  .add('Disabled Content', () => <SyncPage disabled={true} />)
  .add('Enabled Content', () => <SyncPage />)
