/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import SegmentedControl from '@brave/leo/react/segmentedControl'
import SegmentedControlItem from '@brave/leo/react/segmentedControlItem'

import { ColorScheme } from '../api/welcome_api'
import { useWelcomeApi } from '../api/welcome_api_context'
import { useStepTransition } from './use_step_transition'
import { getString } from '../lib/strings'
import { StepHeader } from './step_header'
import { ThemeColorSwatch } from './theme_color_swatch'

import { style } from './appearance_step.style'

interface Props {
  onBack: () => void
  onNext: () => void
}

export function AppearanceStep(props: Props) {
  const api = useWelcomeApi()

  useStepTransition()

  const colorScheme = api.useGetColorScheme().data
  const tabOrientation = api.useGetVerticalTabsEnabled().data
    ? 'vertical'
    : 'horizontal'

  return (
    <div
      data-css-scope={style.scope}
      className='step-view'
    >
      <div className='step-content'>
        <div className='step-text'>
          <StepHeader />
          <h1>Make Brave yours</h1>
          <p>
            Personalize your browsing experience with your preferred tab layout
            and color modes.
          </p>
        </div>
        <div className='step-ui'>
          <div className='tab-layout'>
            <h4>Tab layout</h4>
            <SegmentedControl
              value={tabOrientation}
              onChange={({ value }) => {
                api.setVerticalTabsEnabled([value === 'vertical'])
              }}
            >
              <SegmentedControlItem value='horizontal'>
                <Icon
                  name='window-tabs-horizontal'
                  slot='icon-before'
                />
                Horizontal
              </SegmentedControlItem>
              <SegmentedControlItem value='vertical'>
                <Icon
                  name='window-tabs-vertical-expanded'
                  slot='icon-before'
                />
                Vertical
              </SegmentedControlItem>
            </SegmentedControl>
          </div>
          <div className='theme'>
            <div className='mode'>
              <h4>Theme</h4>
              <SegmentedControl
                value={String(colorScheme)}
                onChange={({ value }) => {
                  api.setColorScheme([Number(value ?? ColorScheme.kSystem)])
                }}
              >
                <SegmentedControlItem value={String(ColorScheme.kSystem)}>
                  <Icon
                    name='theme-system'
                    slot='icon-before'
                  />
                  Device
                </SegmentedControlItem>
                <SegmentedControlItem value={String(ColorScheme.kLight)}>
                  <Icon
                    name='theme-light'
                    slot='icon-before'
                  />
                  Light
                </SegmentedControlItem>
                <SegmentedControlItem value={String(ColorScheme.kDark)}>
                  <Icon
                    name='theme-dark'
                    slot='icon-before'
                  />
                  Dark
                </SegmentedControlItem>
              </SegmentedControl>
            </div>
            <div className='colors'>
              <ThemeColorSwatch
                foregroundColor='rgba(227, 227, 227, 1.00)'
                backgroundColor='rgba(11, 87, 208, 1.00)'
                baseColor='rgba(199, 199, 199, 1.00)'
              />
            </div>
          </div>
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
