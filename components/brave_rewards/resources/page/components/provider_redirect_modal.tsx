/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { externalWalletProviderFromString, getExternalWalletProviderName } from '../../shared/lib/external_wallet'
import { LocaleContext } from '../../shared/lib/locale_context'
import { ModalRedirect } from '../../ui/components'
import * as mojom from '../../shared/lib/mojom'

export function ProviderRedirectModal () {
  const { getString } = React.useContext(LocaleContext)
  const actions = useActions()
  const { modalRedirectProvider, modalRedirect } = useRewardsData((data) => ({
    modalRedirectProvider: data.ui.modalRedirectProvider,
    modalRedirect: data.ui.modalRedirect
  }))

  const walletType =
    externalWalletProviderFromString(modalRedirectProvider) || undefined
  const providerName =
    walletType ? getExternalWalletProviderName(walletType) : ''

  const onClickRetry = () => {
    actions.hideRedirectModal()
    if (walletType) {
      actions.beginExternalWalletLogin(walletType)
    }
  }

  switch (modalRedirect) {
    case 'show':
      return (
        <ModalRedirect
          id={'redirect-modal-show'}
          titleText={getString('processingRequest')}
          walletType={walletType}
        />
      )
    case 'hide':
      return null
    case mojom.ConnectExternalWalletResult.kSuccess:
      return null
    case mojom.ConnectExternalWalletResult.kDeviceLimitReached:
      return (
        <ModalRedirect
          id={'redirect-modal-device-limit-reached'}
          errorText={[getString('redirectModalDeviceLimitReachedText')]}
          titleText={getString('redirectModalDeviceLimitReachedTitle')}
          learnMore={'https://support.brave.com/hc/en-us/articles/360056508071'}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    case mojom.ConnectExternalWalletResult.kFlaggedWallet:
      return (
        <ModalRedirect
          id={'redirect-modal-flagged-wallet'}
          errorText={[
            getString('redirectModalFlaggedWalletText1'),
            getString('redirectModalFlaggedWalletText2'),
            getString('redirectModalFlaggedWalletText3'),
            getString('redirectModalFlaggedWalletText4')]}
          errorTextLink={'https://support.brave.com/hc/en-us/articles/4494596374925'}
          titleText={getString('redirectModalFlaggedWalletTitle')}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    case mojom.ConnectExternalWalletResult.kKYCRequired:
      return (
        <ModalRedirect
          id={'redirect-modal-id-verification-required'}
          errorText={[getString('redirectModalKYCRequiredText').replace('$1', providerName)]}
          titleText={getString('redirectModalKYCRequiredTitle')}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    case mojom.ConnectExternalWalletResult.kMismatchedCountries:
      return (
        <ModalRedirect
          id={'redirect-modal-mismatched-countries'}
          errorText={[getString('redirectModalMismatchedCountriesText').replace('$1', providerName)]}
          titleText={getString('redirectModalMismatchedCountriesTitle')}
          learnMore={'https://support.brave.com/hc/en-us/articles/9809690466061'}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    case mojom.ConnectExternalWalletResult.kMismatchedProviderAccounts:
      return (
        <ModalRedirect
          id={'redirect-modal-mismatched-provider-accounts'}
          errorText={[getString('redirectModalMismatchedProviderAccountsText').replace('$1', providerName)]}
          titleText={getString('redirectModalMismatchedProviderAccountsTitle')}
          learnMore={'https://support.brave.com/hc/en-us/articles/360034841711-What-is-a-verified-wallet-'}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    case mojom.ConnectExternalWalletResult.kProviderUnavailable:
      return (
        <ModalRedirect
          id={'redirect-modal-provider-unavailable'}
          errorText={[
            getString('redirectModalProviderUnavailableText1').replaceAll('$1', providerName),
            getString('redirectModalProviderUnavailableText2')]}
          titleText={getString('redirectModalProviderUnavailableTitle')}
          errorTextLink={'https://status.brave.com/'}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    case mojom.ConnectExternalWalletResult.kRegionNotSupported:
      return (
        <ModalRedirect
          id={'redirect-modal-region-not-supported'}
          errorText={[
            getString('redirectModalRegionNotSupportedText1').replaceAll('$1', providerName),
            getString('redirectModalRegionNotSupportedText2')]}
          titleText={getString('redirectModalRegionNotSupportedTitle')}
          errorTextLink={'https://support.brave.com/hc/en-us/articles/6539887971469'}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    case mojom.ConnectExternalWalletResult.kRequestSignatureVerificationFailure:
      return (
        <ModalRedirect
          id={'redirect-modal-wallet-ownership-verification-failure'}
          errorText={[getString('redirectModalWalletOwnershipVerificationFailureText').replace('$1', providerName)]}
          errorTextLink={'https://community.brave.com'}
          titleText={getString('redirectModalWalletOwnershipVerificationFailureTitle')}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    case mojom.ConnectExternalWalletResult.kUpholdBATNotAllowed:
      return (
        <ModalRedirect
          id={'redirect-modal-uphold-bat-not-allowed'}
          errorText={[getString('redirectModalUpholdBATNotAllowedText')]}
          titleText={getString('redirectModalUpholdBATNotAllowedTitle')}
          learnMore={'https://support.uphold.com/hc/en-us/articles/360033020351-Brave-BAT-and-US-availability'}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    case mojom.ConnectExternalWalletResult.kUpholdInsufficientCapabilities:
      return (
        <ModalRedirect
          id={'redirect-modal-uphold-insufficient-capabilities'}
          errorText={[getString('redirectModalUpholdInsufficientCapabilitiesText')]}
          titleText={getString('redirectModalUpholdInsufficientCapabilitiesTitle')}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    case mojom.ConnectExternalWalletResult.kUpholdTransactionVerificationFailure:
      return (
        <ModalRedirect
          id={'redirect-modal-wallet-ownership-verification-failure'}
          errorText={[getString('redirectModalWalletOwnershipVerificationFailureText').replace('$1', providerName)]}
          errorTextLink={'https://community.brave.com'}
          titleText={getString('redirectModalWalletOwnershipVerificationFailureTitle')}
          buttonText={getString('redirectModalClose')}
          walletType={walletType}
          onClick={actions.hideRedirectModal}
        />
      )
    default:
      // on modalRedirect === 'error', or on an unhandled mojom.ConnectExternalWalletResult
      return (
        <ModalRedirect
          id={'redirect-modal-error'}
          errorText={[getString('redirectModalError')]}
          buttonText={getString('processingRequestButton')}
          titleText={getString('processingRequest')}
          walletType={walletType}
          displayCloseButton={true}
          onClick={onClickRetry}
          onClose={actions.hideRedirectModal}
        />
      )
  }
}
