/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { LayoutContext } from '../lib/layout_context'
import { Modal, ModalCloseButton } from '../../shared/components/modal'
import { SelectProviderCaretIcon } from '../components/icons/select_provider_caret_icon'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { GeminiIcon } from '../../shared/components/icons/gemini_icon'
import { UpholdIcon } from '../../shared/components/icons/uphold_icon'
import { BitflyerIcon } from '../../shared/components/icons/bitflyer_icon'
import { ZebPayIcon } from '../../shared/components/icons/zebpay_icon'
import { supportedWalletRegionsURL } from '../../shared/lib/rewards_urls'

import * as urls from '../../shared/lib/rewards_urls'

import * as style from './connect_wallet_modal.style'

function renderProviderIcon (provider: string) {
  switch (provider) {
    case 'bitflyer': return <BitflyerIcon />
    case 'gemini': return <GeminiIcon />
    case 'uphold': return <UpholdIcon />
    case 'zebpay': return <ZebPayIcon />
    default: return null
  }
}

interface ExternalWalletProvider {
  type: string
  name: string
  enabled: boolean
}

interface Props {
  currentCountryCode: string
  providers: ExternalWalletProvider[]
  onContinue: (provider: string) => void
  onClose: () => void
}

export function ConnectWalletModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const layoutKind = React.useContext(LayoutContext)

  const [selectedProvider, setSelectedProvider] =
    React.useState<ExternalWalletProvider | null>(null)

  if (props.providers.length === 0) {
    return null
  }

  function getDisclaimer () {
    return formatMessage(getString('connectWalletDisclaimer'), {
      tags: {
        $1: (content) => (
          <NewTabLink key='privacy-link' href={urls.privacyPolicyURL}>
            {content}
          </NewTabLink>
        ),
        $3: (content) => (
          <NewTabLink key='bat-link' href={urls.aboutBATURL}>
            {content}
          </NewTabLink>
        )
      }
    })
  }

  function renderConnectWallet () {
    return {
      left: (
        <style.connectWalletLeftPanel>
          <style.panelHeader>
            {getString('connectWalletHeader')}
          </style.panelHeader>
          <style.connectWalletContent>
            <style.panelText>
              <style.panelListItem>
               {getString('connectWalletListItem1')}
              </style.panelListItem>
              {
                // For now, hide the second panel list item about
                // being able to top up if Rewards country is India.
                props.currentCountryCode !== 'IN' &&
                <style.panelListItem>
                  {getString('connectWalletListItem2')}
                </style.panelListItem>
              }
              <style.panelListItem>
                {getString('connectWalletListItem3')}
              </style.panelListItem>
            </style.panelText>
          </style.connectWalletContent>
          {
            layoutKind === 'wide' &&
              <style.connectWalletDisclaimer>
                {getDisclaimer()}
              </style.connectWalletDisclaimer>
          }
        </style.connectWalletLeftPanel>
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
              <style.connectWalletDisclaimer>
                {getDisclaimer()}
              </style.connectWalletDisclaimer>
          }
        </style.providerButtons>
      )
    }
  }

  const { left, right } = renderConnectWallet()

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
