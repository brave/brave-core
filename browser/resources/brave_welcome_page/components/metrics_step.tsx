/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { formatString } from '$web-common/formatString'

import { useWelcomeApi } from '../api/welcome_api_context'
import { useStepTransition } from './use_step_transition'
import { getString } from '../lib/strings'
import { StepHeader } from './step_header'
import { ProductCard } from './product_card'

import { style } from './metrics_step.style'

import wdpImage from '../assets/wdp.svg'
import p3aImage from '../assets/p3a.svg'
import crashesImage from '../assets/crashes.svg'

interface Props {
  onBack: () => void
  onNext: () => void
}

export function MetricsStep(props: Props) {
  const api = useWelcomeApi()

  useStepTransition()

  const webDiscoveryEnabled = api.useGetWebDiscoveryEnabled().data
  const p3aEnabled = api.useGetP3AEnabled().data
  const crashReportsEnabled = api.useGetCrashReportsEnabled().data

  return (
    <div
      data-css-scope={style.scope}
      className='step-view'
    >
      <div className='step-content'>
        <div className='step-text'>
          <StepHeader />
          <h1>{getString('WELCOME_PAGE_METRICS_STEP_TITLE')}</h1>
          <p>{getString('WELCOME_PAGE_METRICS_STEP_TEXT1')}</p>
          <p>
            {formatString(getString('WELCOME_PAGE_METRICS_STEP_TEXT2'), {
              $1: (
                <a
                  href='chrome://settings/privacy'
                  target='_blank'
                  rel='noopener noreferrer'
                >
                  brave://settings/privacy
                </a>
              ),
              $2: (content) => (
                <a
                  href='https://brave.com/privacy/browser/'
                  target='_blank'
                  rel='noopener noreferrer'
                >
                  {content}
                </a>
              ),
            })}
          </p>
        </div>
        <div className='step-ui'>
          <ProductCard
            image={wdpImage}
            title={getString('WELCOME_PAGE_PRODUCT_WDP_TITLE')}
            description={getString('WELCOME_PAGE_PRODUCT_WDP_DESCRIPTION')}
            learnMoreUrl='https://support.brave.app/hc/articles/4409406835469-What-is-the-Web-Discovery-Project'
            checked={webDiscoveryEnabled}
            onChange={(checked) => api.setWebDiscoveryEnabled([checked])}
          />
          <ProductCard
            image={p3aImage}
            title={getString('WELCOME_PAGE_PRODUCT_P3A_TITLE')}
            description={getString('WELCOME_PAGE_PRODUCT_P3A_DESCRIPTION')}
            learnMoreUrl='https://support.brave.app/hc/articles/4409406835469-What-is-the-Web-Discovery-Project'
            checked={p3aEnabled}
            onChange={(checked) => api.setP3AEnabled([checked])}
          />
          <ProductCard
            image={crashesImage}
            title={getString('WELCOME_PAGE_PRODUCT_CRASH_REPORTS_TITLE')}
            description={getString(
              'WELCOME_PAGE_PRODUCT_CRASH_REPORTS_DESCRIPTION',
            )}
            learnMoreUrl='https://support.brave.app/hc/en-us/articles/360017905872-How-do-I-enable-or-disable-automatic-crash-reporting'
            checked={crashReportsEnabled}
            onChange={(checked) => api.setCrashReportsEnabled([checked])}
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
            {getString('WELCOME_PAGE_START_BROWSING_BUTTON_LABEL')}
          </Button>
        </div>
      </footer>
    </div>
  )
}
