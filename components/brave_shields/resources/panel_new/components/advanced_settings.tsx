/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dropdown from '@brave/leo/react/dropdown'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { useShieldsApi } from '../api/shields_api_context'
import { StringKey, getString } from './strings'

import {
  AdBlockMode,
  HttpsUpgradeMode,
  FingerprintMode,
  CookieBlockMode,
  ContentSettingSource,
} from 'gen/brave/components/brave_shields/core/common/shields_settings.mojom.m.js'

import { ContentSetting } from 'gen/components/content_settings/core/common/content_settings.mojom.m'

import { style } from './advanced_settings.style'

interface Props {
  showFingerprintingDetails: () => void
  showAdsBlocked: () => void
  showScriptsBlocked: () => void
}

export function AdvancedSettings(props: Props) {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const showAdvancedView = api.useGetAdvancedViewEnabledData()

  const shieldsEnabled = siteBlockInfo?.isBraveShieldsEnabled
  const adblockOnlyEnabled = siteBlockInfo?.isBraveShieldsAdBlockOnlyModeEnabled

  if (!shieldsEnabled || !showAdvancedView || adblockOnlyEnabled) {
    return null
  }

  return (
    <div
      data-css-scope={style.scope}
      className='scrollable'
      onScroll={(e) => {
        e.currentTarget.toggleAttribute(
          'data-scrolled',
          e.currentTarget.scrollTop > 0,
        )
      }}
    >
      <AdBlockControl showDetails={props.showAdsBlocked} />
      <HttpsUpgradeControls />
      <BlockScriptsControls showDetails={props.showScriptsBlocked} />
      <FingerprintingControls showDetails={props.showFingerprintingDetails} />
      <CookieBlockControls />
      <ForgetFirstPartyStorageControls />
      <BlockElementsControls />
    </div>
  )
}

function AdBlockControl(props: { showDetails: () => void }) {
  const api = useShieldsApi()
  const { data: siteSettings } = api.useGetSiteSettings()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()

  if (!siteSettings || !siteBlockInfo) {
    return null
  }

  const adBlockMode = siteSettings.adBlockMode
  const adsBlocked = siteBlockInfo.adsList.length
  const isManaged = siteBlockInfo.isBraveShieldsManaged

  return (
    <div>
      <Icon name='hand-raised' />
      <Dropdown
        value={String(adBlockMode)}
        disabled={isManaged}
        onChange={({ value }) => {
          withEnumValue(value, (mode) => api.setAdBlockMode([mode]))
        }}
      >
        <span slot='value'>
          {getString(getAdBlockModeLabel(adBlockMode))}
          <Counter count={adsBlocked} />
        </span>
        <leo-option value={String(AdBlockMode.AGGRESSIVE)}>
          {getString(getAdBlockModeLabel(AdBlockMode.AGGRESSIVE))}
        </leo-option>
        <leo-option value={String(AdBlockMode.STANDARD)}>
          {getString(getAdBlockModeLabel(AdBlockMode.STANDARD))}
        </leo-option>
        <leo-option value={String(AdBlockMode.ALLOW)}>
          {getString(getAdBlockModeLabel(AdBlockMode.ALLOW))}
        </leo-option>
      </Dropdown>
      <Button
        onClick={props.showDetails}
        isDisabled={adsBlocked <= 0}
        kind='plain-faint'
        fab
        aria-label={getString('BRAVE_SHIELDS_VIEW_DETAILS')}
      >
        <Icon name='carat-right' />
      </Button>
    </div>
  )
}

function getAdBlockModeLabel(mode: number | undefined) {
  switch (mode) {
    case AdBlockMode.AGGRESSIVE:
      return 'BRAVE_SHIELDS_TRACKERS_AND_ADS_BLOCKED_AGG'
    case AdBlockMode.STANDARD:
      return 'BRAVE_SHIELDS_TRACKERS_AND_ADS_BLOCKED_STD'
    case AdBlockMode.ALLOW:
      return 'BRAVE_SHIELDS_TRACKERS_AND_ADS_ALLOW_ALL'
  }
  throw new Error(`Unrecognized AdBlockMode value: ${mode}`)
}

function HttpsUpgradeControls() {
  const api = useShieldsApi()
  const { data: siteSettings } = api.useGetSiteSettings()
  const isHttpsByDefaultEnabled = api.useIsHttpsByDefaultEnabledData()
  const isTorProfile = api.useIsTorProfileData()

  if (!siteSettings) {
    return null
  }

  const httpsUpgradeMode = siteSettings.httpsUpgradeMode

  if (!isHttpsByDefaultEnabled || isTorProfile) {
    return null
  }

  return (
    <div>
      <Icon name='lock-dots' />
      <Dropdown
        value={String(httpsUpgradeMode)}
        onChange={({ value }) => {
          withEnumValue(value, (mode) => api.setHttpsUpgradeMode([mode]))
        }}
      >
        <leo-option value={String(HttpsUpgradeMode.STRICT_MODE)}>
          {getString('BRAVE_SHIELDS_HTTPS_UPGRADE_MODE_STRICT')}
        </leo-option>
        <leo-option value={String(HttpsUpgradeMode.STANDARD_MODE)}>
          {getString('BRAVE_SHIELDS_HTTPS_UPGRADE_MODE_STANDARD')}
        </leo-option>
        <leo-option value={String(HttpsUpgradeMode.DISABLED_MODE)}>
          {getString('BRAVE_SHIELDS_HTTPS_UPGRADE_MODE_DISABLED')}
        </leo-option>
      </Dropdown>
      <div />
    </div>
  )
}

function BlockScriptsControls(props: { showDetails: () => void }) {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const { data: siteSettings } = api.useGetSiteSettings()

  if (!siteBlockInfo || !siteSettings) {
    return null
  }

  const jsBlocked = siteBlockInfo.blockedJsList.length
  const jsAllowed = siteBlockInfo.allowedJsList.length
  const isNoscriptEnabled = siteSettings.isNoscriptEnabled
  const isManaged = siteBlockInfo.isBraveShieldsManaged
  const hasBlockedOrAllowed = jsBlocked > 0 || jsAllowed > 0

  const overrideStatus = siteSettings.scriptsBlockedOverrideStatus
  const overrideSource = overrideStatus?.overrideSource

  const scriptsBlockedEnforced =
    overrideStatus
    && overrideStatus.status !== ContentSetting.DEFAULT
    && overrideSource !== ContentSettingSource.kUser
    && overrideSource !== ContentSettingSource.kNone

  return (
    <div>
      <Icon name='code' />
      <div className='toggle'>
        <div className='block-scripts-text'>
          <div>
            {getString('BRAVE_SHIELDS_SCRIPTS_BLOCKED')}
            <Counter count={jsBlocked} />
          </div>
          {scriptsBlockedEnforced && (
            <div className='note'>
              {getString(getEnforcedDescription(overrideSource))}
            </div>
          )}
        </div>
        <Toggle
          checked={isNoscriptEnabled}
          disabled={isManaged || scriptsBlockedEnforced}
          size='small'
          onChange={(event) => {
            api.setIsNoScriptsEnabled([event.checked])
          }}
        />
      </div>
      <Button
        onClick={props.showDetails}
        isDisabled={
          !isNoscriptEnabled || !hasBlockedOrAllowed || scriptsBlockedEnforced
        }
        kind='plain-faint'
        fab
        aria-label={getString('BRAVE_SHIELDS_VIEW_DETAILS')}
      >
        <Icon name='carat-right' />
      </Button>
    </div>
  )
}

function getEnforcedDescription(overrideSource: number | undefined): StringKey {
  switch (overrideSource) {
    case ContentSettingSource.kExtension:
      return 'BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN_BY_EXTENSION'
    case ContentSettingSource.kPolicy:
      return 'BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN_BY_POLICY'
    case ContentSettingSource.kAllowList:
      return 'BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN_BY_ALLOWLIST'
    case ContentSettingSource.kSupervised:
      return 'BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN_BY_SUPERVISOR'
    case ContentSettingSource.kInstalledWebApp:
      return 'BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN_BY_PWA'
    default:
      return 'BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN'
  }
}

function FingerprintingControls(props: { showDetails: () => void }) {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const { data: siteSettings } = api.useGetSiteSettings()
  const showStrictMode = api.useShowStrictFingerprintingModeData()
  const webcompatExceptionsEnabled =
    api.useIsWebcompatExceptionsServiceEnabledData()

  if (!siteBlockInfo || !siteSettings) {
    return null
  }

  const isManaged = siteBlockInfo.isBraveShieldsManaged
  const fingerprintMode = siteSettings.fingerprintMode
  const invokedWebcompatCount = siteBlockInfo.invokedWebcompatList.length

  return (
    <div>
      <Icon name='biometric-login' />
      {showStrictMode ? (
        <Dropdown
          value={String(fingerprintMode)}
          disabled={isManaged}
          onChange={({ value }) => {
            withEnumValue(value, (mode) => api.setFingerprintMode([mode]))
          }}
        >
          <leo-option value={String(FingerprintMode.STRICT_MODE)}>
            {getString('BRAVE_SHIELDS_FINGERPRINTING_BLOCKED_AGG')}
          </leo-option>
          <leo-option value={String(FingerprintMode.STANDARD_MODE)}>
            {getString('BRAVE_SHIELDS_FINGERPRINTING_BLOCKED_STD')}
          </leo-option>
          <leo-option value={String(FingerprintMode.ALLOW_MODE)}>
            {getString('BRAVE_SHIELDS_FINGERPRINTING_ALLOW_ALL')}
          </leo-option>
        </Dropdown>
      ) : (
        <div className='toggle'>
          {getString('BRAVE_SHIELDS_FINGERPRINTING_BLOCKED_STD')}
          <Toggle
            checked={fingerprintMode !== FingerprintMode.ALLOW_MODE}
            disabled={isManaged}
            size='small'
            onChange={(event) => {
              api.setFingerprintMode([
                event.checked
                  ? FingerprintMode.STANDARD_MODE
                  : FingerprintMode.ALLOW_MODE,
              ])
            }}
          />
        </div>
      )}
      <Button
        onClick={props.showDetails}
        isDisabled={
          !webcompatExceptionsEnabled
          || invokedWebcompatCount <= 0
          || fingerprintMode === FingerprintMode.ALLOW_MODE
        }
        kind='plain-faint'
        fab
        aria-label={getString('BRAVE_SHIELDS_VIEW_DETAILS')}
      >
        <Icon name='carat-right' />
      </Button>
    </div>
  )
}

function CookieBlockControls() {
  const api = useShieldsApi()
  const { data: siteSettings } = api.useGetSiteSettings()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()

  if (!siteBlockInfo || !siteSettings) {
    return null
  }

  const cookieBlockMode = siteSettings.cookieBlockMode
  const isManaged = siteBlockInfo.isBraveShieldsManaged

  return (
    <div>
      <Icon name='cookie' />
      <Dropdown
        value={String(cookieBlockMode)}
        disabled={isManaged}
        onChange={({ value }) => {
          withEnumValue(value, (mode) => api.setCookieBlockMode([mode]))
        }}
      >
        <leo-option value={String(CookieBlockMode.BLOCKED)}>
          {getString('BRAVE_SHIELDS_COOKIES_BLOCKED')}
        </leo-option>
        <leo-option value={String(CookieBlockMode.CROSS_SITE_BLOCKED)}>
          {getString('BRAVE_SHIELDS_THIRD_PARTY_COOKIES_BLOCKED')}
        </leo-option>
        <leo-option value={String(CookieBlockMode.ALLOW)}>
          {getString('BRAVE_SHIELDS_COOKIES_ALLOWED_ALL')}
        </leo-option>
      </Dropdown>
      <div />
    </div>
  )
}

function ForgetFirstPartyStorageControls() {
  const api = useShieldsApi()
  const { data: siteSettings } = api.useGetSiteSettings()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const forgetFeatureEnabled =
    api.useIsBraveForgetFirstPartyStorageFeatureEnabledData()

  if (!siteBlockInfo || !siteSettings) {
    return null
  }

  const isForgetEnabled = siteSettings.isForgetFirstPartyStorageEnabled
  const isManaged = siteBlockInfo.isBraveShieldsManaged

  if (!forgetFeatureEnabled) {
    return null
  }

  return (
    <div>
      <Icon name='broom' />
      <div className='toggle'>
        {getString('BRAVE_SHIELDS_FORGET_FIRST_PARTY_STORAGE_LABEL')}
        <Toggle
          checked={isForgetEnabled}
          disabled={isManaged}
          size='small'
          onChange={(event) => {
            api.setForgetFirstPartyStorageEnabled([event.checked])
          }}
        />
      </div>
      <div />
    </div>
  )
}

function BlockElementsControls() {
  const api = useShieldsApi()
  const blockedElementsPresent = api.useAreAnyBlockedElementsPresentData()

  if (!blockedElementsPresent) {
    return null
  }

  return (
    <div>
      <Icon name='disable-outline' />
      <div className='block-elements'>
        <div>{getString('BRAVE_SHIELDS_BLOCK_ELEMENTS')}</div>
        <button onClick={() => api.resetBlockedElements()}>
          {getString('BRAVE_SHIELDS_SHOW_ALL_BLOCKED_ELEMENTS')}
        </button>
      </div>
      <div />
    </div>
  )
}

function Counter(props: { count: number }) {
  if (props.count <= 0) {
    return null
  }
  return (
    <span className='counter'>{props.count > 99 ? '99+' : props.count}</span>
  )
}

function withEnumValue(s: string | undefined, fn: (value: number) => void) {
  const value = parseInt(s ?? '', 10)
  if (!isNaN(value)) {
    fn(value)
  }
}
