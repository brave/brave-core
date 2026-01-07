/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useStepTransition } from './use_step_transition'
import { getString } from '../lib/strings'
import { StepHeader } from './step_header'
import { ProductCard } from './product_card'

import { style } from './features_step.style'

import aiImage from '../assets/ai.svg'
import web3Image from '../assets/web3.svg'
import rewardsImage from '../assets/rewards.svg'

interface Props {
  onBack: () => void
  onNext: () => void
}

export function FeaturesStep(props: Props) {
  useStepTransition()

  return (
    <div
      data-css-scope={style.scope}
      className='step-view'
    >
      <div className='step-content'>
        <div className='step-text'>
          <StepHeader />
          <h1>{getString('WELCOME_PAGE_FEATURES_STEP_TITLE')}</h1>
          <p>{getString('WELCOME_PAGE_FEATURES_STEP_TEXT')}</p>
        </div>
        <div className='step-ui'>
          <ProductCard
            image={aiImage}
            title={getString('WELCOME_PAGE_PRODUCT_LEO_AI_TITLE')}
            description={getString('WELCOME_PAGE_PRODUCT_LEO_AI_DESCRIPTION')}
            checked={false}
            onChange={() => {}}
          />
          <ProductCard
            image={web3Image}
            title={getString('WELCOME_PAGE_PRODUCT_WEB3_WALLET_TITLE')}
            description={getString(
              'WELCOME_PAGE_PRODUCT_WEB3_WALLET_DESCRIPTION',
            )}
            checked={false}
            onChange={() => {}}
          />
          <ProductCard
            image={rewardsImage}
            title={getString('WELCOME_PAGE_PRODUCT_REWARDS_TITLE')}
            description={getString('WELCOME_PAGE_PRODUCT_REWARDS_DESCRIPTION')}
            checked={false}
            onChange={() => {}}
          />
        </div>
      </div>
      <footer>
        <div className='back'>
          <Button
            kind='plain-faint'
            size='large'
            onClick={props.onBack}
          >
            {getString('WELCOME_PAGE_BACK_BUTTON_LABEL')}
          </Button>
        </div>
        <div className='forward'>
          <Button
            kind='filled'
            size='large'
            onClick={props.onNext}
          >
            {getString('WELCOME_PAGE_CONTINUE_BUTTON_LABEL')}
          </Button>
        </div>
      </footer>
    </div>
  )
}
