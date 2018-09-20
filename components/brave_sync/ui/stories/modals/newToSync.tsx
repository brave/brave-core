/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
// import Heading from '../../../../src/components/text/heading'
import Button from '../../../../src/components/buttonsIndicators/button'
import Input from '../../../../src/components/formControls/input'
import Modal from '../../../../src/components/popupModals/modal'

// Feature-specific components
import { Title, Label, SectionBlock, FlexColumn } from '../../../../src/features/sync'

// Utils
import locale from '../page/fakeLocale'

interface NewToSyncModalProps {
  onClose: () => void
}

class NewToSyncModal extends React.PureComponent<NewToSyncModalProps, {}> {
  get fakeDeviceName () {
    return 'Your favorite coding OS'
  }

  setupSync = () => {
    console.log('fake: setting up sync')
  }

  render () {
    const { onClose } = this.props
    return (
      <Modal id='showIAmNewToSyncModal' onClose={onClose} size='small'>
        <Title level={1}>{locale.iAmNewToSync}</Title>
        <Label>{locale.enterAnOptionalName}</Label>
        <SectionBlock>
          <Input placeholder={this.fakeDeviceName} />
        </SectionBlock>
        <FlexColumn items='center' content='flex-end'>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={this.setupSync}
            text={locale.setUpSync}
          />
        </FlexColumn>
      </Modal>
    )
  }
}

export default NewToSyncModal
