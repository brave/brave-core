/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

 // Components
import { Button, Modal, TextAreaClipboard } from 'brave-ui'

// Feature-specific components
import {
  ViewSyncCodeGrid,
  ModalTitle,
  TwoColumnButtonGrid
} from 'brave-ui/features/sync'

// Utils
import { getLocale } from '../../../../common/locale'

// Images
import { QRCode } from 'brave-ui/features/sync/images'

interface Props {
  syncData: Sync.State
  actions: any
  onClose: () => void
}

export default class ViewSyncCodeModal extends React.PureComponent<Props, {}> {
  render () {
    const { onClose, syncData } = this.props

    if (!syncData) {
      return null
    }

    return (
      <Modal id='viewSyncCodeModal' onClose={onClose} size='small'>
        <ViewSyncCodeGrid>
          <div style={{ textAlign: 'center' }}>
            <ModalTitle level={3}>{getLocale('qrCode')}</ModalTitle>
            <QRCode size='small' src={syncData.seedQRImageSource} />
          </div>
          <div>
            <ModalTitle level={3}>{getLocale('wordCode')}</ModalTitle>
            <TextAreaClipboard
              copiedString={getLocale('copied')}
              wordCountString={getLocale('wordCount')}
              readOnly={true}
              defaultValue={syncData.syncWords}
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
