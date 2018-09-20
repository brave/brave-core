/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Main,
  Title,
  EmphasisText,
  SecondaryText,
  Link,
  SectionBlock
} from '../../../../src/features/sync'

// Component groups
import DisabledContent from '../disabledContent'
import EnabledContent from '../enabledContent'

// Utils
import locale from './fakeLocale'
import 'emptykit.css'

// Fonts
import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'

interface SyncPageProps {
  // Note: this is for demonstration purposes and
  // should not be included in production
  disabled?: boolean
}

interface SyncPageState {
  enabledContent: boolean
  syncResetModalOpen: boolean
}

const syncLink = 'https://github.com/brave/sync/wiki/Design'

class SyncPage extends React.PureComponent<SyncPageProps, SyncPageState> {
  get fakeShowCurrentSyncPage () {
    return this.props.disabled
      ? <DisabledContent />
      : <EnabledContent />
  }

  render () {
    return (
      <Main>
        <Title level={2}>{locale.sync}</Title>
          <EmphasisText>
            {locale.syncInfo1}
            <Link href={syncLink} target='_blank' rel='noreferrer noopener'>?</Link>
          </EmphasisText>
          <SecondaryText>{locale.syncInfo2}</SecondaryText>
        <SectionBlock>
          {
            this.fakeShowCurrentSyncPage
          }
        </SectionBlock>
      </Main>
    )
  }
}

export default SyncPage
