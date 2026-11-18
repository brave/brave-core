// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as leo from '@brave/nala/tokens/css/variables'
import { useHistory } from 'react-router'
import { useDispatch, useSelector } from 'react-redux'

// Redux
import { WalletPageActions } from '$wallet/page/actions'
import { PageSelectors } from '$wallet/page/selectors'

// Utils
import { getLocale } from '$web-common/locale'

// Components
import { WelcomeAction } from './welcome_action/welcome_action'

// Routes
import { WalletRoutes } from '$wallet/constants/types'

// Page API Proxy
import getWalletPageApiProxy from '$wallet/page/wallet_page_api_proxy'

// Selectors
import { UISelectors } from '$wallet/common/selectors'
import { useSafeUISelector } from '$wallet/common/hooks/use-safe-selector'

// Styled Components
import { Text, Row, Column } from '$wallet/components/shared/style'
import {
  Content,
  WelcomePageWrapper,
  Title,
  Description,
  ActionsContainer,
} from './onboarding_welcome.style'
import { OnboardingBackground, BraveIcon, Heading } from '../onboarding.style'

export const OnboardingWelcome = () => {
  // Routing
  const history = useHistory()

  // Redux
  const dispatch = useDispatch()
  const setupStillInProgress = useSelector(PageSelectors.setupStillInProgress)

  // Selectors
  const isMobile = useSafeUISelector(UISelectors.isMobile)

  // Computed
  const walletIcons = isMobile
    ? [
        'brave-icon-release-color',
        'phantom-color',
        'metamask-color',
        'coinbase-color',
      ]
    : [
        'brave-icon-release-color',
        'phantom-color',
        'metamask-color',
        'coinbase-color',
        'trezor-color',
        'wallet-ledger',
      ]

  // Effects
  React.useEffect(() => {
    // start wallet setup
    if (!setupStillInProgress) {
      dispatch(WalletPageActions.walletSetupComplete(false))
    }
  }, [setupStillInProgress, dispatch])

  return (
    <>
      <OnboardingBackground />
      <WelcomePageWrapper>
        <Content>
          <Row
            gap='6px'
            justifyContent='flex-start'
            marginBottom={leo.spacing['5Xl']}
          >
            <BraveIcon />
            <Heading>{getLocale(S.BRAVE_WALLET_TITLE)}</Heading>
          </Row>
          <Column
            width='100%'
            justifyContent='flex-start'
            alignItems='flex-start'
            gap='12px'
            margin='0px 0px 32px 0px'
          >
            <Title
              variant='heading.display1'
              textColor='primary'
              textAlign='left'
            >
              {getLocale(S.BRAVE_WALLET_ONBOARDING_WELCOME_TITLE)}
            </Title>
            <Description
              variant='large.regular'
              textColor='secondary'
              textAlign='left'
            >
              {getLocale(S.BRAVE_WALLET_ONBOARDING_WELCOME_DESCRIPTION)}
            </Description>
          </Column>
          <ActionsContainer>
            <WelcomeAction
              title={getLocale(
                S.BRAVE_WALLET_ONBOARDING_WELCOME_NEW_ACCOUNT_TITLE,
              )}
              description={getLocale(
                S.BRAVE_WALLET_ONBOARDING_WELCOME_NEW_ACCOUNT_DESCRIPTION,
              )}
              iconName='plus-add'
              onSelect={() => {
                if (isMobile) {
                  getWalletPageApiProxy().pageHandler.showOnboarding(true)
                } else {
                  history.push(WalletRoutes.OnboardingNewWalletTerms)
                }
              }}
            />
            <WelcomeAction
              title={getLocale(S.BRAVE_WALLET_WELCOME_IMPORT_WALLET_TITLE)}
              description={getLocale(
                S.BRAVE_WALLET_ONBOARDING_WELCOME_IMPORT_WALLET_DESCRIPTION,
              )}
              iconName='download'
              walletIcons={walletIcons}
              onSelect={() => {
                if (isMobile) {
                  getWalletPageApiProxy().pageHandler.showOnboarding(false)
                } else {
                  history.push(WalletRoutes.OnboardingImportSelectWalletType)
                }
              }}
            />
          </ActionsContainer>
          <Text
            variant='small.regular'
            textColor='tertiary'
            textAlign='left'
          >
            {getLocale(S.BRAVE_WALLET_WELCOME_COPYRIGHT)}
          </Text>
        </Content>
      </WelcomePageWrapper>
    </>
  )
}
