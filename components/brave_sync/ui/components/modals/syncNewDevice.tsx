/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal, TextArea } from 'brave-ui'

// Feature-specific components
import { Title, List, ListBullet, FlexColumn, QRCode } from 'brave-ui/features/sync'

// Utils
import { getLocale } from '../../../../common/locale'

interface SyncANewDeviceModalProps {
  seedQRImageSource: string
  syncWords: string
  onClose: () => void
  actions: any
}

interface SyncEnabledContentState {
  showQRCode: boolean
  showSyncWords: boolean
  syncWords: string
}

class SyncANewDeviceModal extends React.PureComponent<SyncANewDeviceModalProps, SyncEnabledContentState> {
  constructor (props: SyncANewDeviceModalProps) {
    super(props)
    this.state = {
      showQRCode: false,
      showSyncWords: false,
      syncWords: ''
    }
  }

  onRequestQRCode = () => {
    this.props.actions.onRequestQRCode()
    this.setState({ showQRCode: !this.state.showQRCode })
  }

  onGetUserInputSyncWords = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.setState({ syncWords: event.target.value })
  }

  onRequestSyncWords = () => {
    this.props.actions.onRequestSyncWords()
    this.setState({ showSyncWords: !this.state.showSyncWords })
  }

  render () {
    const { seedQRImageSource, syncWords, onClose } = this.props
    return (
      <Modal id='showIAmSyncANewDeviceModal' onClose={onClose} size='small'>
        <Title level={1}>{getLocale('syncANewDevice')}</Title>
        <List>
          <ListBullet>{getLocale('syncANewDeviceFirstBulletText')}</ListBullet>
          <ListBullet>
            {getLocale('syncANewDeviceSecondBulletText')}
              <Button
                className='syncButton'
                level='secondary'
                type='accent'
                size='small'
                onClick={this.onRequestQRCode}
                text={getLocale('showSecretQRCode')}
              />
              {
                this.state.showQRCode
                  ? <QRCode src={seedQRImageSource} />
                  : null
              }
          </ListBullet>
          <ListBullet>
            {getLocale('syncANewDeviceThirdBulletText')}
              <Button
                className='syncButton'
                level='secondary'
                type='accent'
                size='small'
                onClick={this.onRequestSyncWords}
                text={getLocale('showSecretCodeWords')}
              />
              {
                this.state.showSyncWords
                  ? <TextArea readOnly={true} value={syncWords} />
                  : null
              }
          </ListBullet>
        </List>
        <FlexColumn items='center' content='flex-end'>
          <Button
            level='secondary'
            type='accent'
            size='medium'
            onClick={onClose}
            text={getLocale('done')}
          />
        </FlexColumn>
      </Modal>
    )
  }
}

export default SyncANewDeviceModal
