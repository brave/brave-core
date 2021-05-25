/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { Modal, ModalCloseButton } from '../../shared/components/modal'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { CaretIcon } from '../../shared/components/icons/caret_icon'
import { GeminiIcon } from '../../shared/components/icons/gemini_icon'
import { UpholdIcon } from '../../shared/components/icons/uphold_icon'
import { BitflyerIcon } from '../../shared/components/icons/bitflyer_icon'
import { termsOfServiceURL, privacyPolicyURL } from '../../shared/components/terms_of_service'

import connectWalletGraphic from '../assets/connect_wallet.svg'

import * as style from './connect_wallet_modal.style'

function CheckIcon () {
  return (
    <svg className='icon' viewBox='0 0 32 32'>
      <path d='M13 26a1 1 0 0 1-.67-.26l-9-8.2a1 1 0 0 1 1.34-1.48l8.23 7.49L27.23 6.13A1 1 0 0 1 28.64 6a1 1 0 0 1 .13 1.4l-15 18.24a1 1 0 0 1-.7.36z' />
    </svg>
  )
}

function getMinimumBalance (provider: string) {
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
type TermsState = 'not-checked' | 'needs-check' | 'checked'

interface ExternalWalletProvider {
  type: string
  name: string
}

interface Props {
  rewardsBalance: number
  providers: ExternalWalletProvider[]
  defaultProvider: string
  onContinue: (provider: string) => void
  onClose: () => void
}

export function ConnectWalletModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  const [modalState, setModalState] = React.useState<ModalState>('info')
  const [termsState, setTermsState] = React.useState<TermsState>('not-checked')
  const [selectedProvider, setSelectedProvider] =
    React.useState<ExternalWalletProvider | null>(null)
  const [showMinimumBalanceWarning, setShowMinimumBalanceWarning] =
    React.useState(false)

  const acceptTermsButton = React.useRef<HTMLButtonElement | null>(null)

  if (props.providers.length === 0) {
    return null
  }

  function renderInfo () {
    const toggleTermsChecked = () => setTermsState(termsState === 'checked'
      ? 'not-checked'
      : 'checked')

    const onContinueClick = () => {
      if (termsState === 'checked') {
        setModalState('select')
      } else {
        setTermsState('needs-check')
        if (acceptTermsButton.current) {
          acceptTermsButton.current.focus()
        }
      }
    }

    const onLoginLinkClicked = (evt: React.MouseEvent) => {
      evt.preventDefault()
      props.onContinue(props.defaultProvider)
    }

    return {
      left: (
        <style.infoPanel>
          <style.panelHeader>
            {getString('connectWalletInfoHeader')}
          </style.panelHeader>
          <style.panelText>
            {
              formatMessage(getString('connectWalletInfoText'), {
                tags: {
                  $1: (content) => <em key='em'>{content}</em>
                }
              })
            }
          </style.panelText>
          <style.acceptTerms>
            <style.acceptTermsCheckbox className={termsState}>
              <button ref={acceptTermsButton} onClick={toggleTermsChecked}>
                {termsState === 'checked' ? <CheckIcon /> : null}
              </button>
            </style.acceptTermsCheckbox>
            <style.acceptTermsLabel>
              {
                formatMessage(getString('connectWalletTermsLabel'), {
                  tags: {
                    $1: (content) => (
                      <NewTabLink href={termsOfServiceURL} key='terms'>
                        {content}
                      </NewTabLink>
                    ),
                    $3: (content) => (
                      <NewTabLink href={privacyPolicyURL} key='privacy'>
                        {content}
                      </NewTabLink>
                    )
                  }
                })
              }
            </style.acceptTermsLabel>
          </style.acceptTerms>
          <style.continueButton>
            <button onClick={onContinueClick}>
              {getString('continue')}<CaretIcon direction='right' />
            </button>
          </style.continueButton>
          <style.infoTerms>
            {getString('connectWalletInfoNote')}
          </style.infoTerms>
          {
            props.defaultProvider &&
              <style.alreadyConnected>
                {getString('alreadyConnected')}
                <div className='login'>
                  <a href='#' onClick={onLoginLinkClicked}>
                    {getString('reconnectWallet')}
                  </a>
                </div>
              </style.alreadyConnected>
          }
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
