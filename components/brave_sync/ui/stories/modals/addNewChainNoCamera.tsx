/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Button from '../../../../src/components/buttonsIndicators/button'
import { LoaderIcon } from '../../../../src/components/icons'
import Modal from '../../../../src/components/popupModals/modal'
import TextAreaClipboard from '../../../../src/components/formControls/textareaClipboard'

// Feature-specific components
import {
  ModalHeader,
  Title,
  Paragraph,
  ModalContent,
  TwoColumnButtonGrid,
  OneColumnButtonGrid
} from '../../../../src/features/sync'

// Utils
import { getLocale } from '../page/fakeLocale'
import data from '../page/fakeData'

interface Props {
  onClose: () => void
}

export default class AddNewChainNoCameraModal extends React.PureComponent<Props, {}> {
  render () {
    const { onClose } = this.props
    return (
      <Modal id='addNewChainNoCameraModal' onClose={onClose} size='small'>
        <ModalHeader>
          <div>
            <Title level={1}>{getLocale('chainCode')}</Title>
            <Paragraph>{getLocale('chainCodeDescription')}</Paragraph>
          </div>
        </ModalHeader>
        <ModalContent>
          <TextAreaClipboard
            copiedString='Copied!'
            wordCountString='Word Count:'
            readOnly={true}
            defaultValue={data.passphrase}
          />
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
            level='secondary'
            type='accent'
            size='medium'
            onClick={onClose}
            disabled={true}
            text={getLocale('lookingForDevice')}
            icon={{ position: 'before', image: <LoaderIcon /> }}
          />
        </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
