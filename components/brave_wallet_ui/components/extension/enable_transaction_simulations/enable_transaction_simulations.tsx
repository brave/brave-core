// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { getLocale, getLocaleWithTag } from '../../../../common/locale'

// hooks
import {
  useSetIsTxSimulationOptInStatusMutation //
} from '../../../common/slices/api.slice'

// components
import { LoadingPanel } from '../loading_panel/loading_panel'

// styles
import {
  LeoSquaredButton,
  Row,
  VerticalDivider,
  VerticalSpacer
} from '../../shared/style'
import { Backdrop, Background, FloatingCard } from '../shared-panel-styles'
import {
  BulletPoints,
  CardContent,
  DashedHorizontalLine,
  Header,
  HeadingText,
  IconContainer,
  LearnMoreLink,
  OptionsRow,
  TermsText,
  errorIconColor
} from './enable_transaction_simulations.styles'

const CHANGE_IN_SETTINGS_TEXT = getLocaleWithTag(
  'braveWalletChangeAnytimeInSettings'
)

const TX_SIMULATION_FEATURE_BULLETS = [
  'braveWalletTransactionSimulationSeeEstimates',
  'braveWalletTransactionSimulationDetectMalicious',
  'braveWalletTransactionSimulationDetectPhishing'
].map((key) => <li key={key}>{getLocale(key)}</li>)

const TX_SIMULATION_TERMS_LINK =
  'https://github.com/brave/brave-browser/wiki/Transaction-Simulation'

export const EnableTransactionSimulations: React.FC = () => {
  // mutations
  const [optInOrOut] = useSetIsTxSimulationOptInStatusMutation()

  return (
    <Background>
      <Backdrop>
        <FloatingCard>
          <Header>
            <Row
              width='100%'
              alignItems='center'
              justifyContent='center'
              gap={'4px'}
            >
              <IconContainer>
                <Icon name={'product-brave-wallet'} />
              </IconContainer>
              <DashedHorizontalLine />
              <IconContainer iconColor={errorIconColor}>
                <Icon name={'warning-triangle-outline'} />
              </IconContainer>
            </Row>
          </Header>

          <CardContent>
            <div>
              <HeadingText>
                {getLocale('braveWalletEnableEnhancedTransactionDetailsTitle')}
              </HeadingText>

              <BulletPoints>{TX_SIMULATION_FEATURE_BULLETS}</BulletPoints>

              <VerticalSpacer space={4} />
              <VerticalDivider />
              <VerticalSpacer space={8} />

              <TermsText>
                {getLocale('braveWalletTransactionSimulationTerms')}{' '}
                <LearnMoreLink
                  href={TX_SIMULATION_TERMS_LINK}
                  rel='noopener noreferrer'
                >
                  {getLocale('braveWalletLearnMore')}
                </LearnMoreLink>
              </TermsText>

              <TermsText>
                {CHANGE_IN_SETTINGS_TEXT.beforeTag}
                <strong>{CHANGE_IN_SETTINGS_TEXT.duringTag}</strong>
                {CHANGE_IN_SETTINGS_TEXT.afterTag}
              </TermsText>
            </div>

            <OptionsRow>
              <LeoSquaredButton
                kind='plain-faint'
                onClick={async () => {
                  await optInOrOut(
                    BraveWallet.BlowfishOptInStatus.kDenied
                  ).unwrap()
                }}
              >
                {getLocale('braveWalletButtonNoThanks')}
              </LeoSquaredButton>
              <LeoSquaredButton
                kind='filled'
                onClick={async () => {
                  await optInOrOut(
                    BraveWallet.BlowfishOptInStatus.kAllowed
                  ).unwrap()
                }}
              >
                {getLocale('braveWalletButtonEnable')}
              </LeoSquaredButton>
            </OptionsRow>
          </CardContent>
        </FloatingCard>
      </Backdrop>
    </Background>
  )
}

export const LoadingSimulation: React.FC = () => {
  return <LoadingPanel message={getLocale('braveWalletScanningTransaction')} />
}
