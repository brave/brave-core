// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// constants
import { BraveWallet } from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  useCreateWalletMutation,
  useAddAccountMutation,
  useGetVisibleNetworksQuery,
  useReportOnboardingActionMutation
} from '../../../../common/slices/api.slice'
import { useAccountsQuery } from '../../../../common/slices/api.slice.extra'
import {
  useSafeUISelector,
  useSafeWalletSelector //
} from '../../../../common/hooks/use-safe-selector'
import { UISelectors, WalletSelectors } from '../../../../common/selectors'
import { keyringIdForNewAccount } from '../../../../utils/account-utils'
import { suggestNewAccountName } from '../../../../utils/address-utils'

// components
import {
  NavButton //
} from '../../../../components/extension/buttons/nav-button/index'
import {
  NewPasswordInput,
  NewPasswordValues
} from '../../../../components/shared/password-input/new-password-input'
import { OnboardingStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { CreatingWallet } from '../creating_wallet/creating_wallet'

// styles
import {
  StyledWrapper,
  Title,
  Description,
  NextButtonRow,
  MainWrapper,
  TitleAndDescriptionContainer
} from '../onboarding.style'

interface OnboardingCreatePasswordProps {
  onWalletCreated: () => void
}

export const OnboardingCreatePassword = ({
  onWalletCreated
}: OnboardingCreatePasswordProps) => {
  // redux
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const allowNewWalletFilecoinAccount = useSafeWalletSelector(
    WalletSelectors.allowNewWalletFilecoinAccount
  )
  const isCreatingWallet = useSafeUISelector(UISelectors.isCreatingWallet)

  // state
  const [isValid, setIsValid] = React.useState(false)
  const [password, setPassword] = React.useState('')

  // mutations
  const [createWallet] = useCreateWalletMutation()
  const [addAccount] = useAddAccountMutation()
  const [report] = useReportOnboardingActionMutation()

  // queries
  const { accounts } = useAccountsQuery()
  const { data: visibleNetworks } = useGetVisibleNetworksQuery()
  const visibleNetworkTypes = React.useMemo(() => {
    return visibleNetworks.map((n) => ({
      coin: n.coin,
      symbolName: n.symbolName,
      keyringId: keyringIdForNewAccount(n.coin, n.chainId)
    }))
  }, [visibleNetworks])

  // methods
  const nextStep = React.useCallback(async () => {
    if (!isValid) {
      return
    }
    // Note: intentionally not using unwrapped value
    // results are returned before other redux actions complete
    await createWallet({ password }).unwrap()

    // create accounts for visible network coin types if needed
    for (const netType of visibleNetworkTypes) {
      if (
        // TODO: remove this check when we can hide "default" networks
        netType.coin === BraveWallet.CoinType.FIL &&
        allowNewWalletFilecoinAccount
      ) {
        await addAccount({
          accountName: suggestNewAccountName(accounts, netType),
          coin: netType.coin,
          keyringId: netType.keyringId
        }).unwrap()
      }
    }
  }, [
    isValid,
    createWallet,
    password,
    visibleNetworkTypes,
    allowNewWalletFilecoinAccount,
    addAccount,
    accounts
  ])

  const handlePasswordChange = React.useCallback(
    ({ isValid, password }: NewPasswordValues) => {
      setPassword(password)
      setIsValid(isValid)
    },
    []
  )

  // effects
  React.useEffect(() => {
    report(BraveWallet.OnboardingAction.LegalAndPassword)
  }, [report])

  React.useEffect(() => {
    // wait for redux before redirecting
    // otherwise, the restricted routes in the router will not be available
    if (!isCreatingWallet && isWalletCreated) {
      onWalletCreated()
    }
  }, [isWalletCreated, onWalletCreated, isCreatingWallet])

  if (isCreatingWallet) {
    return <CreatingWallet />
  }

  // render
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>
          <OnboardingStepsNavigation preventSkipAhead />

          <TitleAndDescriptionContainer>
            <Title>{getLocale('braveWalletCreatePasswordTitle')}</Title>
            <Description>
              {getLocale('braveWalletCreatePasswordDescription')}
            </Description>
          </TitleAndDescriptionContainer>

          <NewPasswordInput
            autoFocus={true}
            onSubmit={nextStep}
            onChange={handlePasswordChange}
          />

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonNext')}
              onSubmit={nextStep}
              disabled={!isValid}
            />
          </NextButtonRow>
        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}

export default OnboardingCreatePassword
