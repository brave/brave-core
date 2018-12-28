/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal } from 'brave-ui'
import { LoaderIcon } from 'brave-ui/components/icons'

// Feature-specific components
import {
  ModalHeader,
  Title,
  Bold,
  Paragraph,
  ScanGrid,
  ThreeColumnButtonGrid,
  Link
} from 'brave-ui/features/sync'

// Modals
import ViewSyncCode from './viewSyncCode'

// Utils
import { getLocale } from '../../../../common/locale'

// Images
import { SyncMobilePicture, QRCode } from 'brave-ui/features/sync/images'

interface Props {
  syncData: Sync.State
  actions: any
  onClose: () => void
}
interface State {
  viewSyncCode: boolean
}

export default class ScanCodeModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      viewSyncCode: false
    }
  }

  onClickEnterCodeWordsInstead = () => {
    this.setState({ viewSyncCode: !this.state.viewSyncCode })
  }

  onCancel = (event: React.MouseEvent<HTMLAnchorElement>) => {
    event.preventDefault()
    this.props.onClose()
  }

  render () {
    const { onClose, syncData, actions } = this.props
    const { viewSyncCode } = this.state

    return (
      <Modal id='scanCodeModal' displayCloseButton={false} size='small'>
        {
          viewSyncCode
            ? <ViewSyncCode syncData={syncData} actions={actions} onClose={this.onClickEnterCodeWordsInstead} />
            : null
        }
        <ModalHeader>
          <div>
            <Title level={1}>{getLocale('scanThisCode')}</Title>
            <Paragraph>
              {getLocale('scanThisCodeHowToPartial1')} <Bold>{getLocale('scanThisCodeHowToPartial2')}</Bold> {getLocale('scanThisCodeHowToPartial3')}
            </Paragraph>
          </div>
        </ModalHeader>
        <ScanGrid>
          <div><SyncMobilePicture /></div>
          {
            syncData.seedQRImageSource
              ? <QRCode size='normal' src={syncData.seedQRImageSource} />
              : null
          }
        </ScanGrid>
        <ThreeColumnButtonGrid>
        <div>
            <Link onClick={this.onCancel}>{getLocale('cancel')}</Link>
          </div>
          <div>
            <Button
              level='secondary'
              type='subtle'
              size='medium'
              onClick={this.onClickEnterCodeWordsInstead}
              text={getLocale('viewSyncCode')}
            />
          </div>
          <div>
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
          </div>
        </ThreeColumnButtonGrid>
      </Modal>
    )
  }
}
