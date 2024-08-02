// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { getLocale, getLocaleWithTags } from '../../../../common/locale'
import { openTab } from '../../../utils/routes-utils'

// hooks
import {
  useSetIsTxSimulationOptInStatusMutation //
} from '../../../common/slices/api.slice'

// components
import { LoadingPanel } from '../loading_panel/loading_panel'

// styles
import {
  Column,
  LeoSquaredButton,
  Row,
  VerticalDivider
} from '../../shared/style'
import {
  BulletPoints,
  CardContent,
  DashedHorizontalLine,
  HeadingText,
  IconContainer,
  OptionsRow,
  TermsText,
  Title,
  errorIconColor
} from './enable_transaction_simulations.styles'

const TX_SIMULATION_LEARN_MORE_LINK =
  'https://github.com/brave/brave-browser/wiki/Transaction-Simulation'
const BLOWFISH_PRIVACY_POLICY_URL = 'https://extension.blowfish.xyz/privacy'
const BLOWFISH_TERMS_URL = 'https://extension.blowfish.xyz/terms'

const openTxSimulationLearnMoreUrl = () =>
  openTab(TX_SIMULATION_LEARN_MORE_LINK)

export const EnableTransactionSimulations: React.FC = () => {
  // mutations
  const [optInOrOut] = useSetIsTxSimulationOptInStatusMutation()

  return (
    <>
      <Title>{getLocale('braveWalletEnableTransactionSimulation')}</Title>
      <CardContent>
        <Column>
          <Row
            width='100%'
            alignItems='center'
            justifyContent='center'
            gap={'4px'}
            padding={'55px 0px'}
          >
            <IconContainer>
              <Icon name={'product-brave-wallet'} />
            </IconContainer>
            <DashedHorizontalLine />
            <IconContainer iconColor={errorIconColor}>
              <Icon name={'warning-triangle-outline'} />
            </IconContainer>
          </Row>

          <div>
            <HeadingText>
              {getLocale('braveWalletTransactionSimulationFeatureDescription')}
            </HeadingText>

            <BulletPoints>
              <li key={'estimate'}>
                {getLocale('braveWalletTransactionSimulationSeeEstimates')}
              </li>
              <li key={'detect'}>
                {getLocale('braveWalletTransactionSimulationDetectMalicious')}
              </li>
              <li key={'phishing'}>
                {getLocale('braveWalletTransactionSimulationDetectPhishing')}
                {
                  //
                  ' '
                }
                <a
                  href='#'
                  onClick={openTxSimulationLearnMoreUrl}
                >
                  {getLocale('braveWalletLearnMore')}
                </a>
              </li>
            </BulletPoints>

            <VerticalDivider margin='32px 0px' />

            <TermsText>
              <span>
                {getLocaleWithTags(
                  'braveWalletTransactionSimulationTerms',
                  3
                ).map((text, index) => {
                  return (
                    <span key={text.duringTag ?? index}>
                      {text.beforeTag}
                      {index === 0 ? (
                        <strong>{text.duringTag}</strong>
                      ) : (
                        <a
                          href='#'
                          onClick={() =>
                            index === 1
                              ? openTab(BLOWFISH_TERMS_URL)
                              : openTab(BLOWFISH_PRIVACY_POLICY_URL)
                          }
                        >
                          {text.duringTag}
                        </a>
                      )}
                      {text.afterTag}
                    </span>
                  )
                })}
              </span>
            </TermsText>
          </div>

          <OptionsRow>
            <LeoSquaredButton
              kind='outline'
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
        </Column>
      </CardContent>
    </>
  )
}

export const LoadingSimulation: React.FC = () => {
  return <LoadingPanel message={getLocale('braveWalletScanningTransaction')} />
}
