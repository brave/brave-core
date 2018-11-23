/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Button from '../../../../src/components/buttonsIndicators/button'
import Modal from '../../../../src/components/popupModals/modal'
import TextAreaClipboard from '../../../../src/components/formControls/textareaClipboard'

// Feature-specific components
import {
  ViewSyncCodeGrid,
  ModalTitle,
  TwoColumnButtonGrid
} from '../../../../src/features/sync'

import { QRCode } from '../../../../src/features/sync/images'

// Fake QR Code
import qrCodeImage from '../../../assets/img/fakeQRCodeImage.png'

// Utils
import { getLocale } from '../page/fakeLocale'
import data from '../page/fakeData'

interface Props {
  onClose: () => void
}

export default class ViewSyncCodeModal extends React.PureComponent<Props, {}> {
  render () {
    const { onClose } = this.props
    return (
      <Modal id='viewSyncCodeModal' onClose={onClose} size='small'>
        <ViewSyncCodeGrid>
          <div style={{ textAlign: 'center' }}>
            <ModalTitle level={3}>{getLocale('qrCode')}</ModalTitle>
            <QRCode size='small' src={qrCodeImage} />
          </div>
          <div>
            <ModalTitle level={3}>{getLocale('wordCode')}</ModalTitle>
            <TextAreaClipboard
              copiedString={getLocale('copied')}
              wordCountString={getLocale('wordCount')}
              defaultValue={data.passphrase}
            />
          </div>
        </ViewSyncCodeGrid>
        <TwoColumnButtonGrid>
          <div>{getLocale('privateKey')}</div>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={onClose}
            text={getLocale('done')}
          />
        </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
