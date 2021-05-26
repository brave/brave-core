/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { Modal, ModalCloseButton } from '../../shared/components/modal'
import { CaretIcon } from '../../shared/components/icons/caret_icon'
import { GeminiIcon } from '../../shared/components/icons/gemini_icon'
import { UpholdIcon } from '../../shared/components/icons/uphold_icon'
import { BitflyerIcon } from '../../shared/components/icons/bitflyer_icon'

import connectWalletGraphic from '../assets/connect_wallet.svg'

import * as style from './connect_wallet_modal.style'

export function getMinimumBalance (provider: string) {
  switch (provider) {
    case 'uphold': return 15
    default: return 0
  }
}

function renderProviderIcon (provider: string) {
  switch (provider) {
    case 'bitflyer': return <BitflyerIcon />
    case 'gemini': return <GeminiIcon />
    case 'uphold': return <UpholdIcon />
    default: return null
  }
}

type ModalState = 'info' | 'select'

interface ExternalWalletProvider {
  type: string
  name: string
}

interface Props {
  rewardsBalance: number
  providers: ExternalWalletProvider[]
  onContinue: (provider: string) => void
  onClose: () => void
}

export function ConnectWalletModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  const [modalState, setModalState] = React.useState<ModalState>('info')
  const [selectedProvider, setSelectedProvider] =
    React.useState<ExternalWalletProvider | null>(null)
  const [showMinimumBalanceWarning, setShowMinimumBalanceWarning] =
    React.useState(false)

  if (props.providers.length === 0) {
    return null
  }

  function renderInfo () {
    const onContinueClick = () => {
      setModalState('select')
    }

    return {
      left: (
        <style.infoPanel>
          <style.panelHeader>
            {getString('connectWalletInfoHeader')}
          </style.panelHeader>
          <style.panelText>
            {getString('connectWalletInfoText')}
            <style.infoListItem>
              {getString('connectWalletInfoListItem1')}
            </style.infoListItem>
            <style.infoListItem>
              {getString('connectWalletInfoListItem2')}
            </style.infoListItem>
          </style.panelText>
          <style.infoNote>
            {getString('connectWalletInfoNote')}
          </style.infoNote>
          <style.continueButton>
            <button onClick={onContinueClick}>
              {getString('continue')}<CaretIcon direction='right' />
            </button>
          </style.continueButton>
          <style.infoTerms>
            {
              formatMessage(getString('connectWalletInfoBraveNote'), {
                tags: {
                  $1: (content) => <strong key='1'>{content}</strong>
                }
              })
            }
          </style.infoTerms>
        </style.infoPanel>
      ),
      right: (
        <style.connectGraphic>
          <img src={connectWalletGraphic} />
        </style.connectGraphic>
      )
    }
  }

  function renderSelectWallet () {
    const onLoginLinkClicked = (evt: React.MouseEvent) => {
      evt.preventDefault()
      if (selectedProvider) {
        props.onContinue(selectedProvider.type)
      }
    }

    return {
      left: (
        <style.selectWalletLeftPanel>
          <style.panelHeader>
            {getString('connectWalletChooseHeader')}
          </style.panelHeader>
          <style.selectWalletContent>
            <style.panelText>
              {getString('connectWalletChooseText')}
            </style.panelText>
            {
              showMinimumBalanceWarning && selectedProvider &&
                <style.minimumBalanceWarning>
                  {
                    formatMessage(getString('minimumBalanceWarning'), [
                      selectedProvider.name,
                      getMinimumBalance(selectedProvider.type)
                    ])
                  }
                  <div className='login'>
                    <a href='#' onClick={onLoginLinkClicked}>
                      {getString('continueToLogin')}
                    </a>
                  </div>
                </style.minimumBalanceWarning>
            }
          </style.selectWalletContent>
          <style.selectWalletNote>
            {getString('connectWalletChooseNote')}
          </style.selectWalletNote>
        </style.selectWalletLeftPanel>
      ),
      right: (
        <style.providerButtons>
          {
            props.providers.map((provider) => {
              const onClick = () => {
                setSelectedProvider(provider)
                if (props.rewardsBalance < getMinimumBalance(provider.type)) {
                  setShowMinimumBalanceWarning(true)
                } else {
                  setShowMinimumBalanceWarning(false)
                  props.onContinue(provider.type)
                }
              }

              const selected =
                selectedProvider &&
                provider.type === selectedProvider.type

              return (
                <button
                  key={provider.type}
                  onClick={onClick}
                  className={selected ? 'selected' : ''}
                >
                  <style.providerButtonIcon>
                    {renderProviderIcon(provider.type)}
                  </style.providerButtonIcon>
                  <style.providerButtonName>
                    {provider.name}
                  </style.providerButtonName>
                  <style.providerButtonCaret>
                    <CaretIcon direction='right' />
                  </style.providerButtonCaret>
                </button>
              )
            })
          }
        </style.providerButtons>
      )
    }
  }

  const { left, right } = modalState === 'info'
    ? renderInfo()
    : renderSelectWallet()

  return (
    <Modal>
      <style.root>
        <style.leftPanel>
          {left}
        </style.leftPanel>
        <style.rightPanel>
          <ModalCloseButton onClick={props.onClose} />
          {right}
        </style.rightPanel>
      </style.root>
    </Modal>
  )
}
