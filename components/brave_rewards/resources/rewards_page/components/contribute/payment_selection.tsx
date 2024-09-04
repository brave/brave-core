/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useLocaleContext } from '../../lib/locale_strings'
import { useAppState } from '../../lib/app_model_context'
import { TabOpenerContext , NewTabLink } from '../../../shared/components/new_tab_link'
import { WalletProviderIcon } from '../../../shared/components/icons/wallet_provider_icon'
import { ExternalWalletProvider, isSelfCustodyProvider, getExternalWalletProviderName } from '../../../shared/lib/external_wallet'
import * as urls from '../../../shared/lib/rewards_urls'

import { style } from './payment_selection.style'

interface Props {
  onSelectCustodial: () => void
  onCancel: () => void
}

export function PaymentSelection(props: Props) {
  const { getString } = useLocaleContext()
  const tabOpener = React.useContext(TabOpenerContext)

  const [creator, externalWallet] = useAppState((state) => [
    state.currentCreator,
    state.externalWallet
  ])

  if (!creator) {
    return null
  }

  function getMatchingProvider(): ExternalWalletProvider | '' {
    if (!externalWallet || !creator) {
      return ''
    }
    if (isSelfCustodyProvider(externalWallet.provider)) {
      return ''
    }
    if (creator.supportedWalletProviders.includes(externalWallet.provider)) {
      return externalWallet.provider
    }
    return ''
  }

  function getSupportedProviders() {
    if (!creator) {
      return []
    }
    const providers: ExternalWalletProvider[] = []
    for (const provider of creator.supportedWalletProviders) {
      if (!isSelfCustodyProvider(provider)) {
        providers.push(provider)
      }
    }
    return providers
  }

  const { web3URL } = creator.banner
  const matchingProvider = getMatchingProvider()

  if (!web3URL && !matchingProvider) {
    return (
      <div {...style}>
        <div className='text'>
          <Icon name='hand-coins' />
          <div>
            {getString('contributeAvailableMethodsText')}
          </div>
        </div>
        <div className='methods'>
          {
            getSupportedProviders().map((provider) => (
              <div key={provider} className='method'>
                <div>
                  <div className='provider-icon'>
                    <WalletProviderIcon provider={provider} />
                  </div>
                  {getExternalWalletProviderName(provider)}
                </div>
              </div>
            ))
          }
        </div>
        <div className='actions'>
          <NewTabLink href={urls.tippingLearnMoreURL}>
            {getString('contributeAboutMethodsLink')}
          </NewTabLink>
          <Button kind='plain-faint' onClick={props.onCancel}>
            {getString('cancelButtonLabel')}
          </Button>
        </div>
      </div>
    )
  }

  return (
    <div {...style}>
      <div className='text'>
        <Icon name='hand-coins' />
        <div>{getString('contributeChooseMethodText')}</div>
      </div>
      <div className='methods'>
        {
          web3URL &&
            <div className='method'>
              <button onClick={() => tabOpener.openTab(web3URL)}>
                <div className='provider-icon'>
                  <Icon name='product-brave-wallet' />
                </div>
                <div className='provider-text'>
                  {getString('contributeWeb3Label')}
                  <div className='subtext'>
                    {getString('contributeWeb3Subtext')}
                  </div>
                </div>
              </button>
            </div>
        }
        {
          matchingProvider &&
            <div className='method'>
              <button onClick={props.onSelectCustodial}>
                <div className='provider-icon'>
                  <WalletProviderIcon provider={matchingProvider} />
                </div>
                <div className='provider-text'>
                  {getExternalWalletProviderName(matchingProvider)}
                  <div className='subtext'>
                    {getString('contributeCustodialSubtext')}
                  </div>
                </div>
              </button>
            </div>
        }
      </div>
      <div className='actions'>
        <Button kind='plain-faint' onClick={props.onCancel}>
          {getString('cancelButtonLabel')}
        </Button>
      </div>
    </div>
  )
}
