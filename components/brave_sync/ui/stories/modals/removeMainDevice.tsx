/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Modal, Button } from '../../../../src/components'

// Feature-specific components
import {
  Paragraph,
  ModalHeader,
  Title,
  ModalContent,
  TwoColumnButtonGrid,
  OneColumnButtonGrid
 } from '../../../../src/features/sync'

// Utils
import { getLocale } from '../page/fakeLocale'

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
          <Title level={1}>{getLocale('remove')} “{mainDeviceName}” {getLocale('thisSyncChain')}?</Title>
        </ModalHeader>
        <ModalContent>
          <Paragraph>{getLocale('thisDeviceRemovalDescription')}</Paragraph>
        </ModalContent>
        <TwoColumnButtonGrid>
            <OneColumnButtonGrid>
              <Button
                level='secondary'
                type='subtle'
                size='medium'
                onClick={onClose}
                text={getLocale('cancel')}
              />
            </OneColumnButtonGrid>
            <Button
              level='primary'
              type='warn'
              size='medium'
              onClick={onClose}
              text={getLocale('remove')}
            />
        </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
