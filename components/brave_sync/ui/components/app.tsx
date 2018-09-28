/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Feature-specific components
import {
  Main,
  Title,
  EmphasisText,
  SecondaryText,
  Link,
  SectionBlock
} from 'brave-ui/features/sync'

// Component groups
import DisabledContent from './disabledContent'
import EnabledContent from './enabledContent'

// Utils
import { getLocale } from '../../../common/locale'
import * as syncActions from '../actions/sync_actions'

// Assets
require('../../../fonts/muli.css')
require('../../../fonts/poppins.css')
require('emptykit.css')

interface Props {
  syncData: Sync.State
  actions: any
}

interface State {
  deviceName: string
  showQRCode: boolean
  showSyncWords: boolean
  syncWords: string
}

const syncLink = 'https://github.com/brave/sync/wiki/Design'

export class SyncPage extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      deviceName: '',
      showQRCode: false,
      showSyncWords: false,
      syncWords: ''
    }
  }

  componentDidMount () {
    // Inform the back-end that Sync can be loaded
    syncActions.onPageLoaded()
  }

  render () {
    const { syncData, actions } = this.props
    if (!syncData) {
      return null
    }
    return (
      <div id='syncPage'>
        <Main>
          <Title level={2}>{getLocale('sync')}</Title>
            <EmphasisText>
              {getLocale('syncInfo1')}
              <Link href={syncLink} target='_blank' rel='noreferrer noopener'>?</Link>
            </EmphasisText>
            <SecondaryText>{getLocale('syncInfo2')}</SecondaryText>
          <SectionBlock>
            {
              syncData.isSyncConfigured
                ? <EnabledContent syncData={syncData} actions={actions} />
                : <DisabledContent syncData={syncData} actions={actions} />
            }
          </SectionBlock>
        </Main>
      </div>
    )
  }
}

export const mapStateToProps = (state: Sync.ApplicationState) => ({
  syncData: state.syncData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(syncActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(SyncPage)
