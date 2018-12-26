/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Modal, TextAreaClipboard, Button } from '../../../../src/components'
import { LoaderIcon } from '../../../../src/components/icons'

// Feature-specific components
import {
  ModalHeader,
  Title,
  Paragraph,
  Link,
  Bold,
  ThreeColumnButtonGrid
} from '../../../../src/features/sync'

// Utils
import { getLocale } from '../page/fakeLocale'
import data from '../page/fakeData'

interface Props {
  onClose: (event?: any) => void
}

export default class ViewSyncCodeModal extends React.PureComponent<Props, {}> {
  onCancel = (event: React.MouseEvent<HTMLAnchorElement>) => {
    event.preventDefault()
    this.props.onClose()
  }
  render () {
    const { onClose } = this.props
    return (
      <Modal id='viewSyncCodeModal' displayCloseButton={false} size='small'>
        <ModalHeader>
          <div>
            <Title level={1}>{getLocale('chainCode')}</Title>
            <Paragraph>
              {getLocale('chainCodeDescriptionPartial1')} <Bold>{getLocale('chainCodeDescriptionPartial2')}</Bold> {getLocale('chainCodeDescriptionPartial3')}
            </Paragraph>
          </div>
        </ModalHeader>
        <TextAreaClipboard
          copiedString='Copied!'
          wordCountString='Word Count:'
          readOnly={true}
          defaultValue={data.passphrase}
        />
        <ThreeColumnButtonGrid>
          <div>
            <Link onClick={this.onCancel}>{getLocale('cancel')}</Link>
          </div>
          <div>
            <Button
              level='secondary'
              type='subtle'
              size='medium'
              onClick={onClose}
              text={getLocale('qrCode')}
            />
          </div>
          <Button
            level='secondary'
            type='accent'
            size='medium'
            onClick={onClose}
            disabled={true}
            text={getLocale('lookingForDevice')}
            icon={{ position: 'before', image: <LoaderIcon /> }}
          />
        </ThreeColumnButtonGrid>
      </Modal>
    )
  }
}
