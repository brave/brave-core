/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { TwitterColorIcon, RedditColorIcon } from 'brave-ui/components/icons'

import { TipKind, MediaMetaData } from '../lib/interfaces'
import { HostContext } from '../lib/host_context'
import { Locale, LocaleContext } from '../../shared/lib/locale_context'

import { RewardsTour } from '../../shared/components/onboarding'
import { SliderSwitch, SliderSwitchOption } from './slider_switch'
import { TipComplete } from './tip_complete'
import { OptInForm } from './opt_in_form'
import { OneTimeTipForm } from './one_time_tip_form'
import { MonthlyTipForm } from './monthly_tip_form'

import * as style from './tip_form.style'

function getTipKindOptions (
  locale: Locale,
  showMonthlyStar: boolean
): SliderSwitchOption<TipKind>[] {
  let monthlyText = locale.getString('monthlyText')
  if (showMonthlyStar) {
    monthlyText = '* ' + monthlyText
  }
  return [
    { value: 'one-time', content: locale.getString('oneTimeTip') },
    { value: 'monthly', content: monthlyText }
  ]
}

function getHeaderSubtitle (locale: Locale, mediaMetaData: MediaMetaData) {
  if (mediaMetaData.mediaType === 'twitter') {
    return (
      <style.headerSubtitle>
        {locale.getString('tipPostSubtitle')}&nbsp;
        <style.socialIcon>
          <TwitterColorIcon />
        </style.socialIcon>
      </style.headerSubtitle>
    )
  }

  if (mediaMetaData.mediaType === 'reddit') {
    return (
      <style.headerSubtitle>
        {locale.getString('tipPostSubtitle')}&nbsp;
        <style.socialIcon><RedditColorIcon /></style.socialIcon>
      </style.headerSubtitle>
    )
  }

  return ''
}

export function TipForm () {
  const host = React.useContext(HostContext)
  const locale = React.useContext(LocaleContext)
  const { getString } = locale

  const [balanceInfo, setBalanceInfo] = React.useState(
    host.state.balanceInfo)
  const [publisherInfo, setPublisherInfo] = React.useState(
    host.state.publisherInfo)
  const [rewardsParameters, setRewardsParameters] = React.useState(
    host.state.rewardsParameters)
  const [showOnboarding, setShowOnboarding] = React.useState(
    host.state.showOnboarding)
  const [showTour, setShowTour] = React.useState(false)
  const [currentMonthlyTip, setCurrentMonthlyTip] = React.useState(
    host.state.currentMonthlyTip || 0)
  const [autoContributeAmount, setAutoContributeAmount] = React.useState(
    host.state.autoContributeAmount || 0)
  const [adsPerHour, setAdsPerHour] = React.useState(
    host.state.adsPerHour || 0)
  const [walletInfo, setWalletInfo] = React.useState(
    host.state.externalWalletInfo)

  const [tipAmount, setTipAmount] = React.useState(0)
  const [tipProcessed, setTipProcessed] = React.useState(false)
  const [wasMonthlySelected, setWasMonthlySelected] = React.useState(false)

  const [tipKind, setTipKind] = React.useState<TipKind>(() => {
    const { entryPoint } = host.getDialogArgs()
    return entryPoint === 'one-time' ? 'one-time' : 'monthly'
  })

  React.useEffect(() => {
    return host.addListener((state) => {
      setTipProcessed(Boolean(state.tipProcessed))
      setTipAmount(state.tipAmount || 0)
      setRewardsParameters(state.rewardsParameters)
      setPublisherInfo(state.publisherInfo)
      setBalanceInfo(state.balanceInfo)
      setShowOnboarding(state.showOnboarding)
      setCurrentMonthlyTip(state.currentMonthlyTip || 0)
      setAutoContributeAmount(state.autoContributeAmount || 0)
      setAdsPerHour(state.adsPerHour || 0)
      setWalletInfo(state.externalWalletInfo)
    })
  }, [host])

  if (!rewardsParameters || !publisherInfo || !balanceInfo || !walletInfo) {
    return <style.loading />
  }

  if (tipProcessed) {
    return <TipComplete tipKind={tipKind} tipAmount={tipAmount} />
  }

  if (showTour) {
    const externalWalletType = walletInfo ? walletInfo.type : ''

    // Hide AC options in rewards onboarding for bitFlyer-associated regions.
    let { autoContributeChoices } = rewardsParameters
    if (externalWalletType === 'bitflyer') {
      autoContributeChoices = []
    }

    const onTourDone = () => setShowTour(false)

    const onVerifyClick = () => {
      window.open('chrome://rewards/#verify', '_blank')
      setShowTour(false)
    }

    return (
      <style.tour>
        <RewardsTour
          firstTimeSetup={!showOnboarding}
          adsPerHour={adsPerHour}
          externalWalletProvider={externalWalletType}
          autoContributeAmount={autoContributeAmount}
          autoContributeAmountOptions={autoContributeChoices}
          onAdsPerHourChanged={host.setAdsPerHour}
          onAutoContributeAmountChanged={host.setAutoContributeAmount}
          onVerifyWalletClick={onVerifyClick}
          onDone={onTourDone}
        />
      </style.tour>
    )
  }

  if (showOnboarding) {
    const onTakeTour = () => {
      setShowTour(true)
    }

    const onEnable = () => {
      host.saveOnboardingResult('opted-in')
      onTakeTour()
    }

    const onDismiss = () => {
      host.saveOnboardingResult('dismissed')
    }

    return (
      <OptInForm
        onTakeTour={onTakeTour}
        onEnable={onEnable}
        onDismiss={onDismiss}
      />
    )
  }

  const { mediaMetaData } = host.getDialogArgs()

  function onTipKindSelect (value: TipKind) {
    if (value === 'monthly') {
      setWasMonthlySelected(true)
    }
    setTipKind(value)
  }

  function getTipKindSelector () {
    const { mediaType } = mediaMetaData
    if (mediaType === 'twitter' || mediaType === 'reddit') {
      return null
    }

    const showMonthlyIndicator =
      !wasMonthlySelected &&
      host.getDialogArgs().entryPoint === 'one-time' &&
      currentMonthlyTip > 0

    return (
      <>
        {
          showMonthlyIndicator &&
            <style.monthlyIndicator>
              <style.monthlyIndicatorStar>*</style.monthlyIndicatorStar>&nbsp;
              {getString('currentlySupporting')}
            </style.monthlyIndicator>
        }
        <SliderSwitch<TipKind>
          options={getTipKindOptions(locale, showMonthlyIndicator)}
          selectedValue={tipKind}
          onSelect={onTipKindSelect}
        />
      </>
    )
  }

  return (
    <style.root>
      <style.header>
        {getString('supportThisCreator')}
        {getHeaderSubtitle(locale, mediaMetaData)}
      </style.header>
      <style.tipKind>
        {getTipKindSelector()}
      </style.tipKind>
      <style.main>
        {tipKind === 'one-time' ? <OneTimeTipForm /> : <MonthlyTipForm />}
      </style.main>
    </style.root>
  )
}
