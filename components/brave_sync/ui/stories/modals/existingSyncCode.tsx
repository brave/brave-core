/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Button from '../../../../src/components/buttonsIndicators/button'
import Input from '../../../../src/components/formControls/input'
import Modal from '../../../../src/components/popupModals/modal'
import TextArea from '../../../../src/components/formControls/textarea'

// Feature-specific components
import { Title, Label, SectionBlock, FlexColumn } from '../../../../src/features/sync'

// Utils
import locale from '../page/fakeLocale'

interface ExistingSyncCodeModalProps {
  onClose: () => void
}

class ExistingSyncCodeModal extends React.PureComponent<ExistingSyncCodeModalProps, {}> {
  get fakeDeviceName () {
    return 'Your favorite coding OS'
  }

  setUpSync = () => {
    console.log('fake: setting up sync')
  }

  render () {
    const { onClose } = this.props
    return (
      <Modal id='showIAmExistingSyncCodeModal' onClose={onClose} size='small'>
        <Title level={1}>{locale.iHaveAnExistingSyncCode}</Title>
        <Label>{locale.enterYourSyncCodeWords}</Label>
        <SectionBlock>
          <TextArea />
        </SectionBlock>
        <Label>{locale.enterAnOptionalNameForThisDevice}</Label>
        <SectionBlock>
          <Input placeholder={this.fakeDeviceName} />
        </SectionBlock>
        <FlexColumn items='center' content='flex-end'>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={this.setUpSync}
            text={locale.setUpSync}
          />
        </FlexColumn>
      </Modal>
    )
  }
}

export default ExistingSyncCodeModal
