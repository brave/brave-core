/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Button from '../../../../src/components/buttonsIndicators/button'
import Modal from '../../../../src/components/popupModals/modal'
import TextAreaClipboard from '../../../../src/components/formControls/textareaClipboard'

// Feature-specific components
import { ModalHeader, ModalTitle, ModalSubTitle, ModalContent, ThreeColumnButtonGrid, ThreeColumnButtonGridCol2, ThreeColumnButtonGridCol1 } from '../../../../src/features/sync'

// Modals
import ScanCode from './scanCode'

// Utils
import { getLocale } from '../page/fakeLocale'
import data from '../page/fakeData'

interface Props {
  fromMobileScreen?: boolean
  onClose: () => void
}

interface State {
  useCameraInstead: boolean
}

export default class AddNewChainCameraOptionModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      useCameraInstead: false
    }
  }

  onClickUseCameraInsteadButton = () => {
    this.setState({ useCameraInstead: !this.state.useCameraInstead })
  }

  render () {
    const { fromMobileScreen, onClose } = this.props
    const { useCameraInstead } = this.state
    return (
      <Modal id='addNewChainCameraOptionModal' onClose={onClose} size='small'>
        {
          useCameraInstead
            ? <ScanCode onClose={this.onClickUseCameraInsteadButton} />
            : null
        }
        <ModalHeader>
          <div>
            <ModalTitle level={1}>
              {
                fromMobileScreen
                  ? getLocale('mobileEnterThisCode')
                  : getLocale('enterThisCode')
              }
            </ModalTitle>
            <ModalSubTitle>{getLocale('syncChainCodeHowTo')}</ModalSubTitle>
          </div>
        </ModalHeader>
        <ModalContent>
          <TextAreaClipboard
            copiedString='Copied!'
            wordCountString={getLocale('wordCount')}
            readOnly={true}
            defaultValue={data.passphrase}
          />
        </ModalContent>
        <ThreeColumnButtonGrid>
          <ThreeColumnButtonGridCol1>
            <Button
              level='secondary'
              type='accent'
              size='medium'
              onClick={this.onClickUseCameraInsteadButton}
              text={getLocale('useCameraInstead')}
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
              disabled={true}
              text={getLocale('lookingForDevice')}
            />
          </ThreeColumnButtonGridCol2>
        </ThreeColumnButtonGrid>
      </Modal>
    )
  }
}
