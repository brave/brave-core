/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Button from '../../../../src/components/buttonsIndicators/button'
import Modal from '../../../../src/components/popupModals/modal'

// Feature-specific components
import {
  Paragraph,
  ModalHeader,
  // ModalIcon,
  ModalTitle,
  ModalContent,
  TwoColumnButtonGrid,
  OneColumnButtonGrid
 } from '../../../../src/features/sync'

// Utils
import { getLocale } from '../page/fakeLocale'

// Images
import { SyncRemoveIcon } from '../../../../src/features/sync/images'

interface Props {
  mainDeviceName: string
  onClose: () => void
}

export default class RemoveMainDeviceModal extends React.PureComponent<Props, {}> {
  render () {
    const { onClose, mainDeviceName } = this.props
    return (
      <Modal id='removeMainDeviceModal' onClose={onClose} size='small'>
        <ModalHeader>
          <SyncRemoveIcon />
          <ModalTitle level={1}>{getLocale('remove')} “{mainDeviceName}” {getLocale('thisSyncChain')}?</ModalTitle>
        </ModalHeader>
        <ModalContent>
          <Paragraph>{getLocale('thisDeviceRemovalDescription')}</Paragraph>
          <Paragraph>{getLocale('joinSyncChain')}</Paragraph>
        </ModalContent>
        <TwoColumnButtonGrid>
            <OneColumnButtonGrid>
              <Button
                level='secondary'
                type='accent'
                size='medium'
                onClick={onClose}
                text={getLocale('cancel')}
              />
            </OneColumnButtonGrid>
            <Button
              level='primary'
              type='accent'
              size='medium'
              onClick={onClose}
              text={getLocale('remove')}
            />
        </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
