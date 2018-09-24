/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal, TextArea } from 'brave-ui'

// Feature-specific components
import {
  Title,
  List,
  ListBullet,
  FlexColumn,
  QRCode
} from 'brave-ui/features/sync'

// Utils
import { getLocale } from '../../../../common/locale'

// Assets
// QR CODE GOES HERE
// const fakeQRCodeImage = require('../../../assets/img/fakeQRCodeImage.png')

interface SyncANewDeviceModalProps {
  onClose: () => void
}

interface SyncEnabledContentState {
  showQRCodeModal: boolean
  showCodeWordsModal: boolean
}

class SyncANewDeviceModal extends React.PureComponent<SyncANewDeviceModalProps, SyncEnabledContentState> {
  constructor (props: SyncANewDeviceModalProps) {
    super(props)
    this.state = {
      showQRCodeModal: false,
      showCodeWordsModal: false
    }
  }

  get fakeDeviceName () {
    return 'Your favorite coding OS'
  }

  get fakePassphrase () {
    return [
      'intercepting', 'novelising', 'audited', 'reheeling', 'bone', 'zag', 'cupping', 'gothic',
      'dicky', 'regulation', 'reheard', 'refinished', 'wrenching', 'reinduced', 'wimple', 'welfare'
    ].join(' ')
  }

  showQRCode = () => {
    this.setState({ showQRCodeModal: !this.state.showQRCodeModal })
  }

  showCodeWords = () => {
    this.setState({ showCodeWordsModal: !this.state.showCodeWordsModal })
  }

  render () {
    const { onClose } = this.props
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
                onClick={this.showQRCode}
                text={getLocale('showSecretQRCode')}
              />
              {
                this.state.showQRCodeModal
                  ? <div><QRCode src='' /></div>
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
                onClick={this.showCodeWords}
                text={getLocale('showSecretCodeWords')}
              />
              {
                this.state.showCodeWordsModal
                  ? <TextArea readOnly={true} value={this.fakePassphrase} />
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
