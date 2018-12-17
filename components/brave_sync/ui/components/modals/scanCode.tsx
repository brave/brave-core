/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal } from 'brave-ui'

// Feature-specific components
import {
  ModalHeader,
  ModalTitle,
  ModalSubTitle,
  ScanGrid,
  ThreeColumnButtonGrid,
  ThreeColumnButtonGridCol1,
  ThreeColumnButtonGridCol2
} from 'brave-ui/features/sync'

// Modals
import AddNewChainCameraOptionModal from './addNewChainCameraOption'

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
  enterCodeWordsInstead: boolean
}

export default class ScanCodeModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      enterCodeWordsInstead: false
    }
  }

  onClickEnterCodeWordsInstead = () => {
    this.setState({ enterCodeWordsInstead: !this.state.enterCodeWordsInstead })
  }

  render () {
    const { onClose, syncData, actions } = this.props
    const { enterCodeWordsInstead } = this.state

    return (
      <Modal id='scanCodeModal' onClose={onClose} size='small'>
        {
          enterCodeWordsInstead
            ? <AddNewChainCameraOptionModal syncData={syncData} actions={actions} fromMobileScreen={true} onClose={this.onClickEnterCodeWordsInstead} />
            : null
        }
        <ModalHeader>
          <div>
            <ModalTitle level={1}>{getLocale('scanThisCode')}</ModalTitle>
            <ModalSubTitle>{getLocale('scanThisCodeHowTo')}</ModalSubTitle>
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
          <ThreeColumnButtonGridCol1>
            <Button
              level='secondary'
              type='accent'
              size='medium'
              onClick={this.onClickEnterCodeWordsInstead}
              text={getLocale('enterCodeWordsInstead')}
            />
          </ThreeColumnButtonGridCol1>
          <ThreeColumnButtonGridCol2>
            <Button
              level='secondary'
              type='accent'
              size='medium'
              onClick={onClose}
              text={getLocale('previous')}
            />
            <Button
              level='primary'
              type='accent'
              size='medium'
              onClick={onClose}
              disabled={!syncData.isSyncConfigured}
              text={
                !syncData.isSyncConfigured
                ? getLocale('lookingForDevice')
                : getLocale('ok')
              }
            />
          </ThreeColumnButtonGridCol2>
        </ThreeColumnButtonGrid>
      </Modal>
    )
  }
}
