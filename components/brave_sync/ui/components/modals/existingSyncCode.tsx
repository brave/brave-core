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
import { getDefaultDeviceName } from '../../helpers'

interface ExistingSyncCodeModalProps {
  onClose: () => void
  actions: any
}

interface ExistingSyncCodeModalState {
  deviceName: string
  syncWords: string
}

class ExistingSyncCodeModal extends React.PureComponent<ExistingSyncCodeModalProps, ExistingSyncCodeModalState> {
  constructor (props: ExistingSyncCodeModalProps) {
    super(props)
    this.state = {
      deviceName: '',
      syncWords: ''
    }
  }
  get defaultDeviceName () {
    return getDefaultDeviceName()
  }

  getUserInputDeviceName = (e: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ deviceName: e.target.value })
  }

  getUserInputSyncWords = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.setState({ syncWords: e.target.value })
  }

  onSetupSyncHaveCode = () => {
    const { deviceName, syncWords } = this.state
    this.props.actions.onSetupSyncHaveCode(syncWords, deviceName)
  }

  render () {
    const { onClose } = this.props
    return (
      <Modal id='showIAmExistingSyncCodeModal' onClose={onClose} size='small'>
        <Title level={1}>{getLocale('iHaveAnExistingSyncCode')}</Title>
        <Label>{getLocale('enterYourSyncCodeWords')}</Label>
        <SectionBlock>
          <TextArea onChange={this.getUserInputSyncWords} />
        </SectionBlock>
        <Label>{getLocale('enterAnOptionalName')}</Label>
        <SectionBlock>
          <Input
            placeholder={this.defaultDeviceName}
            onChange={this.getUserInputDeviceName}
          />
        </SectionBlock>
        <FlexColumn items='center' content='flex-end'>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={this.onSetupSyncHaveCode}
            text={getLocale('setUpSync')}
          />
        </FlexColumn>
      </Modal>
    )
  }
}

export default ExistingSyncCodeModal
