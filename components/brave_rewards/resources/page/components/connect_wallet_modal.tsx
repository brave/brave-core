/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { LayoutContext } from '../lib/layout_context'
import { Modal, ModalCloseButton } from '../../shared/components/modal'
import { SelectProviderCaretIcon } from '../components/icons/select_provider_caret_icon'
import { ArrowNextIcon } from '../../shared/components/icons/arrow_next_icon'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { GeminiIcon } from '../../shared/components/icons/gemini_icon'
import { UpholdIcon } from '../../shared/components/icons/uphold_icon'
import { BitflyerIcon } from '../../shared/components/icons/bitflyer_icon'
import { supportedWalletRegionsURL } from '../../shared/lib/rewards_urls'

import * as urls from '../../shared/lib/rewards_urls'

import * as style from './connect_wallet_modal.style'

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
  enabled: boolean
}

interface Props {
  rewardsBalance: number
  providers: ExternalWalletProvider[]
  onContinue: (provider: string) => void
  onClose: () => void
}

export function ConnectWalletModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const layoutKind = React.useContext(LayoutContext)

  const [modalState, setModalState] = React.useState<ModalState>('info')
  const [selectedProvider, setSelectedProvider] =
    React.useState<ExternalWalletProvider | null>(null)

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
            <style.infoListItem>
              {getString('connectWalletInfoListItem3')}
            </style.infoListItem>
          </style.panelText>
          {layoutKind === 'narrow' && <style.connectGraphic />}
          <style.continueButton>
            <button
              data-test-id='connect-continue-button'
              onClick={onContinueClick}
            >
              {getString('rewardsConnectAccount')}<ArrowNextIcon />
            </button>
          </style.continueButton>
          <style.infoNote>
            {
              formatMessage(getString('connectWalletInfoNote'), {
                tags: {
                  $1: (content) => (
                    <NewTabLink key='link' href={urls.privacyPolicyURL}>
                      {content}
                    </NewTabLink>
                  )
                }
              })
            }
          </style.infoNote>
        </style.infoPanel>
      ),
      right: layoutKind === 'wide' && <style.connectGraphic/>
    }
  }

  function renderSelectWallet () {
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
          </style.selectWalletContent>
          {
            layoutKind === 'wide' &&
              <style.selectWalletNote>
                {getString('connectWalletChooseNote')}
              </style.selectWalletNote>
          }
        </style.selectWalletLeftPanel>
      ),
      right: (
        <style.providerButtons>
          {
            props.providers.map((provider) => {
              const onClick = () => {
                if (provider.enabled) {
                  setSelectedProvider(provider)
                  props.onContinue(provider.type)
                }
              }

              const selected =
                selectedProvider &&
                provider.type === selectedProvider.type

              return (
                <button
                  data-test-id='connect-provider-button'
                  key={provider.type}
                  onClick={onClick}
                  className={!provider.enabled ? 'disabled' : selected ? 'selected' : ''}
                >
                  <style.providerButtonGrid>
                    <style.providerButtonIcon>
                      {renderProviderIcon(provider.type)}
                    </style.providerButtonIcon>
                    <style.providerButtonName>
                      {provider.name}
                    </style.providerButtonName>
                    {!provider.enabled &&
                      <style.providerButtonMessage>
                        {getString('connectWalletProviderNotAvailable')}
                      </style.providerButtonMessage>}
                    {provider.enabled &&
                      <style.providerButtonCaret>
                        <SelectProviderCaretIcon />
                      </style.providerButtonCaret>}
                  </style.providerButtonGrid>
                </button>
              )
            })
          }
          <style.learnMoreLink>
            <NewTabLink href={supportedWalletRegionsURL}>
              {getString('connectWalletLearnMore')}
            </NewTabLink>
          </style.learnMoreLink>
          {
            layoutKind === 'narrow' &&
              <style.selectWalletNote>
                {getString('connectWalletChooseNote')}
              </style.selectWalletNote>
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
        <style.close>
          <ModalCloseButton onClick={props.onClose} />
        </style.close>
        <style.leftPanel>
          {left}
        </style.leftPanel>
        <style.rightPanel>
          {right}
        </style.rightPanel>
      </style.root>
    </Modal>
  )
}
