/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal, TextAreaClipboard } from 'brave-ui'
import { LoaderIcon } from 'brave-ui/components/icons'

// Feature-specific components
import {
  ModalHeader,
  Title,
  Paragraph,
  ThreeColumnButtonGrid,
  Bold,
  Link
} from 'brave-ui/features/sync'

// Modals
import ScanCode from './scanCode'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  syncData: Sync.State
  actions: any
  onClose: () => void
}

interface State {
  useCameraInstead: boolean
}

export default class ViewSyncCodeModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      useCameraInstead: false
    }
  }

  onClickUseCameraInsteadButton = () => {
    this.setState({ useCameraInstead: !this.state.useCameraInstead })
  }

  onCancel = (event: React.MouseEvent<HTMLAnchorElement>) => {
    event.preventDefault()
    this.props.onClose()
  }

  render () {
    const { onClose, actions, syncData } = this.props
    const { useCameraInstead } = this.state

    return (
      <Modal id='viewSyncCodeModal' displayCloseButton={false} size='small'>
        {
          useCameraInstead
            ? <ScanCode syncData={syncData} actions={actions} onClose={this.onClickUseCameraInsteadButton} />
            : null
        }
        <ModalHeader>
          <div>
            <Title level={1}>{getLocale('chainCode')}</Title>
            <Paragraph>
              {getLocale('chainCodeDescriptionPartial1')} <Bold>{getLocale('chainCodeDescriptionPartial2')}</Bold> {getLocale('chainCodeDescriptionPartial3')}
            </Paragraph>
          </div>
        </ModalHeader>
          {
            syncData.syncWords
            ? (
              <TextAreaClipboard
                copiedString={getLocale('copied')}
                wordCountString={getLocale('wordCount')}
                readOnly={true}
                defaultValue={syncData.syncWords}
              />
            )
            : null
          }
        <ThreeColumnButtonGrid>
          <div>
            <Link onClick={this.onCancel}>{getLocale('cancel')}</Link>
          </div>
          <div>
            <Button
              level='secondary'
              type='subtle'
              size='medium'
              onClick={this.onClickUseCameraInsteadButton}
              text={getLocale('qrCode')}
            />
          </div>
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
            icon={{
              position: 'before',
              image: syncData.devices.length < 2
                ? <LoaderIcon />
                : null
            }}
          />
        </ThreeColumnButtonGrid>
      </Modal>
    )
  }
}
