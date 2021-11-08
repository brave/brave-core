/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { ExternalWalletProvider } from '../../shared/lib/external_wallet'
import { Modal, ModalCloseButton } from '../../shared/components/modal'
import { TokenAmount } from '../../shared/components/token_amount'
import { SliderSwitch } from '../../shared/components/slider_switch'
import { ConfirmationBox } from './confirmation_box'
import { ActionButton, CancelButton } from './action_button'
import { LinkedDevicesView, LinkedDevice } from './linked_devices_view'

import * as style from './manage_wallet_modal.style'

type SettingsTab = 'backup' | 'restore' | 'reset'

interface Props {
  tokenBalance: number
  paymentID: string
  externalWalletProvider: ExternalWalletProvider | null
  linkedDevices: LinkedDevice[]
  openDeviceSlots: number
  unlinkingAvailableAt: number
  unlinkingInterval: number
  onClose: () => void
  onReset: () => void
  onViewQR: () => void
  onRestoreWallet: (key: string) => void
  onVerifyWallet: () => void
  onUnlinkDevice: (paymentID: string) => void
}

export function ManageWalletModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [currentTab, setCurrentTab] = React.useState<SettingsTab>('backup')
  const [recoveryKeyValue, setRecoveryKeyValue] = React.useState('')
  const [confirmReset, setConfirmReset] = React.useState(false)

  function renderBackupTab () {
    return (
      <>
        <style.sectionText>
          {getString('rewardsBackupNoticeText1')}
        </style.sectionText>
        <style.sectionText>
          {
            formatMessage(getString('rewardsBackupNoticeText2'), {
              tags: {
                $1: (content) => (
                  <a
                    key='verify'
                    data-test-id='backup-verify-link'
                    onClick={props.onVerifyWallet}
                  >
                    {content}
                  </a>
                )
              }
            })
          }
        </style.sectionText>
        <style.formAction>
          <ActionButton onClick={props.onClose}>
            {getString('manageWalletDone')}
          </ActionButton>
        </style.formAction>
      </>
    )
  }

  function renderRestoreTab () {
    const onFileUpload = (event: React.ChangeEvent<HTMLInputElement>) => {
      const { target } = event

      if (!target.files || target.files.length === 0) {
        return
      }

      const reader = new FileReader()

      reader.onload = () => {
        const key = reader.result ? String(reader.result) : ''
        props.onRestoreWallet(key.trim())
      }

      try {
        reader.readAsText(target.files[0])
      } catch {
        props.onRestoreWallet('')
      }
    }

    const onRestoreClick = () => {
      props.onRestoreWallet(recoveryKeyValue.trim())
    }

    const onTextChange = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
      setRecoveryKeyValue(event.target.value)
    }

    return (
      <>
        <style.sectionText>
          {
            formatMessage(getString('manageWalletEnterRecoveryKey'), {
              tags: {
                $1: (content) => (
                  <label key='import'>
                    {content}
                    <input
                      type='file'
                      style={{ display: 'none' }}
                      onChange={onFileUpload}
                    />
                  </label>
                )
              }
            })
          }
        </style.sectionText>
        <style.recoveryInput>
          <textarea onChange={onTextChange}>{recoveryKeyValue}</textarea>
        </style.recoveryInput>
        <style.sectionText>
          {getString('rewardsRestoreText3')}
        </style.sectionText>
        <style.sectionText>
          {
            formatMessage(getString('manageWalletViewQR'), {
              tags: {
                $1: (content) => (
                  <a key='link' onClick={props.onViewQR}>{content}</a>
                )
              }
            })
          }
        </style.sectionText>
        <style.formAction>
          <CancelButton onClick={props.onClose}>
            {getString('cancel')}
          </CancelButton>
          <ActionButton onClick={onRestoreClick}>
            {getString('manageWalletRestore')}
          </ActionButton>
        </style.formAction>
      </>
    )
  }

  function renderResetTab () {
    const text = () => {
      if (props.tokenBalance <= 0) {
        return getString('rewardsResetTextNoFunds')
      }

      return formatMessage(getString('rewardsResetTextFunds'), {
        placeholders: {
          $1: (
            <strong key='amount'>
              <TokenAmount
                amount={props.tokenBalance}
                minimumFractionDigits={0}
              />
            </strong>
          )
        },
        tags: {
          $2: (content) => (
            <a key='link' onClick={props.onVerifyWallet}>
              {content}
            </a>
          )
        }
      })
    }

    const onClick = () => {
      setConfirmReset(true)
    }

    const onConfirm = () => {
      setConfirmReset(false)
      props.onReset()
    }

    const onCancel = () => {
      setConfirmReset(false)
    }

    return (
      <>
        <style.sectionText>
          {text()}
        </style.sectionText>
        <style.formAction>
          <ActionButton onClick={onClick}>
            {getString('manageWalletReset')}
          </ActionButton>
        </style.formAction>
        {
          confirmReset &&
            <ConfirmationBox
              header={''}
              text={getString('rewardsResetConfirmation')}
              buttonText={getString('manageWalletReset')}
              onConfirm={onConfirm}
              onCancel={onCancel}
            />
        }
      </>
    )
  }

  function renderSettingsTab () {
    switch (currentTab) {
      case 'backup': return renderBackupTab()
      case 'restore': return renderRestoreTab()
      case 'reset': return renderResetTab()
    }
  }

  return (
    <Modal>
      <style.root>
        <style.header>{getString('manageWalletHeader')}</style.header>
        <style.paymentIDSection>
          <style.paymentIDBox>
            {getString('manageWalletYourPaymentID')} :&nbsp;
            <style.paymentIDValue>
              {props.paymentID}
            </style.paymentIDValue>
          </style.paymentIDBox>
        </style.paymentIDSection>
        <style.devicesSection>
          <style.sectionHeader>
            {getString('manageWalletDevicesHeader')}
          </style.sectionHeader>
          <LinkedDevicesView
            paymentID={props.paymentID}
            externalWalletProvider={props.externalWalletProvider}
            linkedDevices={props.linkedDevices}
            openDeviceSlots={props.openDeviceSlots}
            unlinkingAvailableAt={props.unlinkingAvailableAt}
            unlinkingInterval={props.unlinkingInterval}
            onVerifyWallet={props.onVerifyWallet}
            onUnlinkDevice={props.onUnlinkDevice}
          />
        </style.devicesSection>
        <style.settingsSection>
          <style.sectionHeader>
            {getString('manageWalletSettingsHeader')}
          </style.sectionHeader>
          <style.tabstrip>
            <SliderSwitch<SettingsTab>
              options={[
                { value: 'backup', content: getString('manageWalletBackup') },
                { value: 'restore', content: getString('manageWalletRestore') },
                { value: 'reset', content: getString('manageWalletReset') }
              ]}
              selectedValue={currentTab}
              onSelect={setCurrentTab}
            />
          </style.tabstrip>
          {renderSettingsTab()}
        </style.settingsSection>
        <style.close>
          <ModalCloseButton onClick={props.onClose} />
        </style.close>
      </style.root>
    </Modal>
  )
}
