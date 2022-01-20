import * as React from 'react'

import * as S from './style'
import Toggle from '../../../../../web-components/toggle'
import { getLocale } from '../../../../../common/locale'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import { useTrackerOptions } from './hooks'

function GlobalSettings () {
  const handleAnchorClick = (e: React.MouseEvent<HTMLAnchorElement>) => {
    e.preventDefault()
    const target = e.target as HTMLAnchorElement
    getPanelBrowserAPI().panelHandler.openURL({ url: target.href })
  }

  return (
    <S.SettingsBox>
      <S.SettingsTitle>{getLocale('braveShieldsGlobalSettingsTitle')}</S.SettingsTitle>
      <a
        href='chrome://settings/shields'
        onClick={handleAnchorClick}
      >
        <S.GlobeIcon />
        {getLocale('braveShieldsChangeDefaults')}
      </a>
      <a
        href='chrome://adblock'
        onClick={handleAnchorClick}
      >
        <S.ListIcon />
        {getLocale('braveShieldsCustomizeAdblockLists')}
      </a>
    </S.SettingsBox>
  )
}

function AdvancedControlsContent () {
  const { adControlType, adControlTypeOptions, handleAdControlTypeChange } = useTrackerOptions()

  const cookiesOptions = [
    { text: getLocale('braveShieldsCrossCookiesBlocked') },
    { text: getLocale('braveShieldsCookiesBlocked') },
    { text: getLocale('braveShieldsCookiesBlockedAll') }
  ]

  const fingerprintingOptions = [
    { text: getLocale('braveShieldsFingerprintingBlockedStd') },
    { text: getLocale('braveShieldsFingerprintingBlockedAgg') },
    { text: getLocale('braveShieldsFingerprintingAllowAll') }
  ]

  return (
    <section
      id='advanced-controls-content'
      aria-label={getLocale('braveShieldsAdvancedCtrls')}
    >
      <S.SettingsBox>
        <S.SettingsTitle>brave.com</S.SettingsTitle>
        <S.SettingsDesc>{getLocale('braveShieldSettingsDescription')}</S.SettingsDesc>
        <S.ControlGroup>
          <S.ControlCount
            aria-label={getLocale('braveShieldsTrackersAndAds')}
            aria-expanded='false'
          >
            <S.CaratDown />
            <span>10</span>
          </S.ControlCount>
          <select
            value={adControlType}
            aria-label={getLocale('braveShieldsTrackersAndAds')}
            onChange={handleAdControlTypeChange}
          >
            {adControlTypeOptions.map((entry, i) => {
              return (
                <option value={entry.value} key={i}>
                  {entry.text}
                </option>
              )
            })}
          </select>
        </S.ControlGroup>
        <S.ControlGroup>
          <S.ControlCount
            aria-label={getLocale('braveShieldsConnectionsUpgraded')}
            aria-expanded='false'
            disabled
          >
            <S.CaratDown />
            <span>0</span>
          </S.ControlCount>
          <label>
            <span>{getLocale('braveShieldsConnectionsUpgraded')}</span>
            <Toggle
              size='sm'
              accessibleLabel='Enable HTTPS'
            />
          </label>
        </S.ControlGroup>
        <S.ControlGroup>
          <S.ControlCount
            aria-label={getLocale('braveShieldsScriptsBlocked')}
            aria-expanded='false'
            disabled
          >
            <S.CaratDown />
            <span>0</span>
          </S.ControlCount>
          <label>
            <span>{getLocale('braveShieldsScriptsBlocked')}</span>
            <Toggle
              size='sm'
              accessibleLabel={getLocale('braveShieldsScriptsBlockedEnable')}
            />
          </label>
        </S.ControlGroup>
        <S.ControlGroup>
          <S.ControlCount disabled />
          <select aria-label={getLocale('braveShieldsCookiesBlocked')}>
              {cookiesOptions.map((entry, i) => {
                return (
                  <option key={i}>
                    {entry.text}
                  </option>
                )
              })}
          </select>
        </S.ControlGroup>
        <S.ControlGroup>
          <S.ControlCount
            aria-label={getLocale('braveShieldsFingerprintingBlocked')}
            aria-expanded='false'
            disabled
          >
            <S.CaratDown />
            <span>0</span>
          </S.ControlCount>
          <select aria-label={getLocale('braveShieldsFingerprintingBlocked')}>
            {fingerprintingOptions.map((entry, i) => {
                return (
                  <option key={i}>
                    {entry.text}
                  </option>
                )
              })}
          </select>
        </S.ControlGroup>
      </S.SettingsBox>
      <GlobalSettings />
  </section>
  )
}

export default AdvancedControlsContent
