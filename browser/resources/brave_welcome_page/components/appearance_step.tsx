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
import { ThemeColorSelector } from './theme_color_selector'

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
          <h1>{getString('WELCOME_PAGE_APPEARANCE_STEP_TITLE')}</h1>
          <p>{getString('WELCOME_PAGE_APPEARANCE_STEP_TEXT')}</p>
        </div>
        <div className='step-ui'>
          <div className='tab-layout'>
            <h4>{getString('WELCOME_PAGE_TAB_LAYOUT_LABEL')}</h4>
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
                {getString('WELCOME_PAGE_TAB_LAYOUT_HORIZONTAL_LABEL')}
              </SegmentedControlItem>
              <SegmentedControlItem value='vertical'>
                <Icon
                  name='window-tabs-vertical-expanded'
                  slot='icon-before'
                />
                {getString('WELCOME_PAGE_TAB_LAYOUT_VERTICAL_LABEL')}
              </SegmentedControlItem>
            </SegmentedControl>
          </div>
          <div className='theme'>
            <div className='mode'>
              <h4>{getString('WELCOME_PAGE_THEME_LABEL')}</h4>
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
                  {getString('WELCOME_PAGE_THEME_SYSTEM_LABEL')}
                </SegmentedControlItem>
                <SegmentedControlItem value={String(ColorScheme.kLight)}>
                  <Icon
                    name='theme-light'
                    slot='icon-before'
                  />
                  {getString('WELCOME_PAGE_THEME_LIGHT_LABEL')}
                </SegmentedControlItem>
                <SegmentedControlItem value={String(ColorScheme.kDark)}>
                  <Icon
                    name='theme-dark'
                    slot='icon-before'
                  />
                  {getString('WELCOME_PAGE_THEME_DARK_LABEL')}
                </SegmentedControlItem>
              </SegmentedControl>
            </div>
            <ThemeColorSelector />
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
