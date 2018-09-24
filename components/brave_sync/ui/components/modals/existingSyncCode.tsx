/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Input, Modal, TextArea } from 'brave-ui'

// Feature-specific components
import { Title, Label, SectionBlock, FlexColumn } from 'brave-ui/features/sync'

// Utils
import { getLocale } from '../../../../common/locale'

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
        <Title level={1}>{getLocale('iHaveAnExistingSyncCode')}</Title>
        <Label>{getLocale('enterYourSyncCodeWords')}</Label>
        <SectionBlock>
          <TextArea />
        </SectionBlock>
        <Label>{getLocale('enterAnOptionalName')}</Label>
        <SectionBlock>
          <Input placeholder={this.fakeDeviceName} />
        </SectionBlock>
        <FlexColumn items='center' content='flex-end'>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={this.setUpSync}
            text={getLocale('setUpSync')}
          />
        </FlexColumn>
      </Modal>
    )
  }
}

export default ExistingSyncCodeModal
