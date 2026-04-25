/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components';

import { LocaleContext, createLocaleContextForTesting } from '../../../lib/locale_context'
import { WithThemeVariables } from '../../with_theme_variables'

import { RewardsOptIn } from '../rewards_opt_in'
import { SettingsOptInForm } from '../settings_opt_in_form'

import { localeStrings } from './locale_strings'

const localeContext = createLocaleContextForTesting(localeStrings)

function actionLogger (name: string) {
  return (...args: any[]) => {
    console.log(name, ...args)
  }
}

interface StoryWrapperProps {
  style?: React.CSSProperties
  children: React.ReactNode
}

const style = {
  wrapper: styled.div`
    border-radius: 8px;
    box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
    width: 410px;
  `
}

function StoryWrapper (props: StoryWrapperProps) {
  return (
    <LocaleContext.Provider value={localeContext}>
      <WithThemeVariables>
        <style.wrapper style={{...props.style || {}}}>
          {props.children}
        </style.wrapper>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

export default {
  title: 'Rewards/Onboarding'
}

export function OptIn() {
  return (
    <StoryWrapper>
      <RewardsOptIn
        initialView='default'
        availableCountries={['US']}
        defaultCountry={'US'}
        result={null}
        onEnable={actionLogger('onEnable')}
        onHideResult={actionLogger('onHideResult')}
      />
    </StoryWrapper>
  )
}

export function OptInDeclareCountry() {
  return (
    <StoryWrapper>
      <RewardsOptIn
        initialView='declare-country'
        availableCountries={['US']}
        defaultCountry={'US'}
        result={null}
        onEnable={actionLogger('onEnable')}
        onHideResult={actionLogger('onHideResult')}
      />
    </StoryWrapper>
  )
}

OptInDeclareCountry.storyName = 'Opt In (declare country)'

export function OptInSuccess () {
  return (
    <StoryWrapper>
      <RewardsOptIn
        initialView='declare-country'
        availableCountries={['US']}
        defaultCountry={'US'}
        result={'success'}
        onEnable={actionLogger('onEnable')}
        onHideResult={actionLogger('onHideResult')}
      />
    </StoryWrapper>
  )
}

OptInSuccess.storyName = 'Opt In (success)'

export function OptInError1() {
  return (
    <StoryWrapper>
      <RewardsOptIn
        initialView='declare-country'
        availableCountries={['US']}
        defaultCountry={'US'}
        result={'wallet-generation-disabled'}
        onEnable={actionLogger('onEnable')}
        onHideResult={actionLogger('onHideResult')}
      />
    </StoryWrapper>
  )
}

OptInError1.storyName = 'Opt In (Error: wallet generation disabled)'

export function OptInError2() {
  return (
    <StoryWrapper>
      <RewardsOptIn
        initialView='declare-country'
        availableCountries={['US']}
        defaultCountry={'US'}
        result={'country-already-declared'}
        onEnable={actionLogger('onEnable')}
        onHideResult={actionLogger('onHideResult')}
      />
    </StoryWrapper>
  )
}

OptInError2.storyName = 'Opt In (Error: country already declared)'

export function OptInError3() {
  return (
    <StoryWrapper>
      <RewardsOptIn
        initialView='declare-country'
        availableCountries={['US']}
        defaultCountry={'US'}
        result={'unexpected-error'}
        onEnable={actionLogger('onEnable')}
        onHideResult={actionLogger('onHideResult')}
      />
    </StoryWrapper>
  )
}

OptInError3.storyName = 'Opt In (Error: unexpected error)'

export function SettingsOptIn () {
  return (
    <StoryWrapper style={{ width: '600px' }}>
      <SettingsOptInForm
        onEnable={actionLogger('onEnable')}
      />
    </StoryWrapper>
  )
}
