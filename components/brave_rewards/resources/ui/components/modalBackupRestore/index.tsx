/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledContent,
  StyledImport,
  StyleButtonWrapper,
  StyledActionsWrapper,
  StyledDoneWrapper
} from './style'
import Tabs from '../../../components/layout/tabs/index'
import TextArea from '../../../components/formControls/textarea/index'
import Modal from '../../../components/popupModals/modal/index'
import ButtonPrimary from '../../../components/buttonsIndicators/buttonPrimary/index'
import ButtonSecondary from '../../../components/buttonsIndicators/buttonSecondary/index'
import { getLocale } from '../../../helpers'

export type TabsType = 'backup' | 'restore'

export interface Props {
  recoveryKey: string
  activeTabId: TabsType
  onTabChange: (tab: TabsType) => void
  onClose: () => void
  onCopy: (key: string) => void
  onPrint: (key: string) => void
  onSaveFile: (key: string) => void
  onRestore: (key: string) => void
  onImport: () => void
  error?: React.ReactNode
  id?: string
}

/*
  TODO
  - add error flow
 */
export default class ModalBackupRestore extends React.PureComponent<Props, {}> {
  render () {
    const {
      id,
      recoveryKey,
      activeTabId,
      onClose,
      onTabChange,
      onCopy,
      onPrint,
      onSaveFile,
      onRestore,
      onImport,
      error
    } = this.props

    return (
      <Modal id={id} onClose={onClose} theme={{ maxWidth: '666px' }}>
        <StyledWrapper>
          <Tabs activeTabId={activeTabId} onChange={onTabChange}>
          <div id={`${id}-backup`} data-key={'backup'} data-title={getLocale('rewardsBackupText1')}>
            <StyledContent>
              {getLocale('rewardsBackupText2')}
            </StyledContent>
            <TextArea
              title={getLocale('recoveryKeys')}
              theme={{ maxWidth: '100%', minHeight: '112px' }}
              defaultValue={recoveryKey}
              disabled={true}
            />
            <StyleButtonWrapper>
              <ButtonSecondary
                text={getLocale('copy')}
                size={'small'}
                color={'subtle'}
                onClick={onCopy.bind(this, recoveryKey)}
              />
              <ButtonSecondary
                text={getLocale('print')}
                size={'small'}
                color={'subtle'}
                onClick={onPrint.bind(this, recoveryKey)}
              />
              <ButtonSecondary
                text={getLocale('saveAsFile')}
                size={'small'}
                color={'subtle'}
                onClick={onSaveFile.bind(this, recoveryKey)}
              />
            </StyleButtonWrapper>
            <StyledDoneWrapper>
              <ButtonPrimary
                text={getLocale('done')}
                size={'medium'}
                color={'brand'}
                onClick={onClose}
              />
            </StyledDoneWrapper>
          </div>
          <div id={`${id}-restore`} data-key={'restore'} data-title={getLocale('rewardsRestoreText1')}>
            <StyledContent>
              {getLocale('rewardsRestoreText2')}
            </StyledContent>
            {
              error
              ? <div>TODO: {error}</div>
              : null
            }
            <TextArea
              title={<>
                {getLocale('rewardsRestoreText3')}<StyledImport onClick={onImport}>{getLocale('import')}</StyledImport>
              </>}
              theme={{ maxWidth: '100%', minHeight: '112px' }}
              defaultValue={''}
            />
            <StyledActionsWrapper>
              <ButtonSecondary
                text={getLocale('cancel')}
                size={'medium'}
                color={'brand'}
                onClick={onClose}
              />
              <ButtonPrimary
                text={getLocale('restore')}
                size={'medium'}
                color={'brand'}
                onClick={onRestore.bind(this, recoveryKey)}
              />
            </StyledActionsWrapper>
          </div>
        </Tabs>
        </StyledWrapper>
      </Modal>
    )
  }
}
