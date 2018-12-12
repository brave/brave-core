/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal, TextAreaClipboard } from 'brave-ui'

// Feature-specific components
import {
  ModalHeader,
  ModalTitle,
  ModalSubTitle,
  ModalContent,
  TwoColumnButtonGrid,
  OneColumnButtonGrid
} from 'brave-ui/features/sync'

// Utils
import { getLocale } from '../../../../common/locale'

// Images
import { SyncAddIcon } from 'brave-ui/features/sync/images'

interface Props {
  syncData: Sync.State
  actions: any
  onClose: () => void
}

export default class AddNewChainNoCameraModal extends React.PureComponent<Props, {}> {
  render () {
    const { onClose, syncData } = this.props

    if (!syncData) {
      return null
    }

    return (
      <Modal id='addNewChainNoCameraModal' onClose={onClose} size='small'>
        <ModalHeader>
          <SyncAddIcon />
          <div>
            <ModalTitle level={1}>{getLocale('enterThisCode')}</ModalTitle>
            <ModalSubTitle>{getLocale('syncChainCodeHowTo')}</ModalSubTitle>
          </div>
        </ModalHeader>
        <ModalContent>
          <TextAreaClipboard
            copiedString={getLocale('copied')}
            wordCountString={getLocale('wordCount')}
            readOnly={true}
            defaultValue={syncData.syncWords}
          />
        </ModalContent>
        <TwoColumnButtonGrid>
          <OneColumnButtonGrid>
            <Button
              level='secondary'
              type='accent'
              size='medium'
              onClick={onClose}
              text={getLocale('previous')}
            />
          </OneColumnButtonGrid>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={onClose}
            disabled={syncData.devices.length < 2}
            text={
              syncData.devices.length < 2
              ? getLocale('lookingForDevice')
              : getLocale('ok')
            }
          />
        </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
