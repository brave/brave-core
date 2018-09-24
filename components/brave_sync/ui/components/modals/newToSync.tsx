/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Input, Modal } from 'brave-ui'

// Feature-specific components
import { Title, Label, SectionBlock, FlexColumn } from 'brave-ui/features/sync'

// Utils
import { getLocale } from '../../../../common/locale'
import { getDefaultDeviceName } from '../../helpers'

interface NewToSyncModalProps {
  onClose: () => void
  actions: any
}

interface NewToSyncModalState {
  deviceName: string
}

class NewToSyncModal extends React.PureComponent<NewToSyncModalProps, NewToSyncModalState> {
  constructor (props: NewToSyncModalProps) {
    super(props)
    this.state = { deviceName: '' }
  }

  get deviceName () {
    return this.state.deviceName === ''
      ? getDefaultDeviceName()
      : this.state.deviceName
  }

  onGetUserInputDeviceName = (e: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ deviceName: e.target.value })
  }

  onSetupNewToSync = () => {
    this.props.actions.onSetupNewToSync(this.state.deviceName)
  }

  render () {
    const { onClose } = this.props
    const { deviceName } = this.state
    return (
      <Modal id='showIAmNewToSyncModal' onClose={onClose} size='small'>
        <Title level={1}>{getLocale('iAmNewToSync')}</Title>
        <Label>{getLocale('enterAnOptionalName')}</Label>
        <SectionBlock>
          <Input
            placeholder={this.deviceName}
            value={deviceName}
            onChange={this.onGetUserInputDeviceName}
          />
        </SectionBlock>
        <FlexColumn items='center' content='flex-end'>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={this.onSetupNewToSync}
            text={getLocale('setUpSync')}
          />
        </FlexColumn>
      </Modal>
    )
  }
}

export default NewToSyncModal
