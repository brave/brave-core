/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { useShieldsState, useShieldsActions } from '../lib/shields_context'
import { getString } from '../lib/strings'

import {
  AdBlockMode,
  HttpsUpgradeMode,
  ContentSetting,
  ContentSettingSource,
  FingerprintMode,
  CookieBlockMode,
} from '../lib/shields_store'

import { style } from './advanced_settings.style'

interface Props {
  showFingerprintingDetails: () => void
  showAdsBlocked: () => void
  showScriptsBlocked: () => void
}

export function AdvancedSettings(props: Props) {
  const shieldsEnabled = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsEnabled,
  )
  const adblockOnlyEnabled = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsAdBlockOnlyModeEnabled,
  )
  const showAdvancedSettings = useShieldsState((s) => s.showAdvancedSettings)

  if (!shieldsEnabled || !showAdvancedSettings || adblockOnlyEnabled) {
    return null
  }

  return (
    <div
      data-css-scope={style.scope}
      data-expanded={showAdvancedSettings}
      className='scrollable'
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
  const actions = useShieldsActions()
  const adBlockMode = useShieldsState((s) => s.siteSettings.adBlockMode)
  const adsBlocked = useShieldsState((s) => s.siteBlockInfo.adsList.length)
  const isManaged = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsManaged,
  )
  return (
    <div>
      <Icon name='hand-raised' />
      <select
        value={adBlockMode}
        disabled={isManaged}
        onChange={(event) => {
          actions.setAdBlockMode(parseInt(event.currentTarget.value))
        }}
      >
        <SelectButton>
          <Counter count={adsBlocked} />
        </SelectButton>
        <option value={AdBlockMode.AGGRESSIVE}>
          {getString('BRAVE_SHIELDS_TRACKERS_AND_ADS_BLOCKED_AGG')}
        </option>
        <option value={AdBlockMode.STANDARD}>
          {getString('BRAVE_SHIELDS_TRACKERS_AND_ADS_BLOCKED_STD')}
        </option>
        <option value={AdBlockMode.ALLOW}>
          {getString('BRAVE_SHIELDS_TRACKERS_AND_ADS_ALLOW_ALL')}
        </option>
      </select>
      <button
        onClick={props.showDetails}
        disabled={adsBlocked <= 0}
      >
        <Icon name='carat-right' />
      </button>
    </div>
  )
}

function HttpsUpgradeControls() {
  const actions = useShieldsActions()
  const httpsUpgradeMode = useShieldsState(
    (s) => s.siteSettings.httpsUpgradeMode,
  )
  const isHttpsByDefaultEnabled = useShieldsState(
    (s) => s.isHttpsByDefaultEnabled,
  )
  const isTorProfile = useShieldsState((s) => s.isTorProfile)

  if (!isHttpsByDefaultEnabled || isTorProfile) {
    return null
  }

  return (
    <div>
      <Icon name='lock-dots' />
      <select
        value={httpsUpgradeMode}
        onChange={(event) => {
          actions.setHttpsUpgradeMode(parseInt(event.currentTarget.value))
        }}
      >
        <SelectButton />
        <option value={HttpsUpgradeMode.STRICT_MODE}>
          {getString('BRAVE_SHIELDS_HTTPS_UPGRADE_MODE_STRICT')}
        </option>
        <option value={HttpsUpgradeMode.STANDARD_MODE}>
          {getString('BRAVE_SHIELDS_HTTPS_UPGRADE_MODE_STANDARD')}
        </option>
        <option value={HttpsUpgradeMode.DISABLED_MODE}>
          {getString('BRAVE_SHIELDS_HTTPS_UPGRADE_MODE_DISABLED')}
        </option>
      </select>
      <div />
    </div>
  )
}

function BlockScriptsControls(props: { showDetails: () => void }) {
  const actions = useShieldsActions()
  const jsBlocked = useShieldsState((s) => s.siteBlockInfo.blockedJsList.length)
  const jsAllowed = useShieldsState((s) => s.siteBlockInfo.allowedJsList.length)
  const isNoscriptEnabled = useShieldsState(
    (s) => s.siteSettings.isNoscriptEnabled,
  )
  const isManaged = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsManaged,
  )
  const hasBlockedOrAllowed = jsBlocked > 0 || jsAllowed > 0

  const overrideSource = useShieldsState(
    (s) => s.siteSettings.scriptsBlockedOverrideStatus?.overrideSource,
  )

  const scriptsBlockedEnforced = useShieldsState((s) => {
    const overrideStatus = s.siteSettings.scriptsBlockedOverrideStatus
    if (!overrideStatus) {
      return false
    }
    if (
      overrideStatus.status === ContentSetting.DEFAULT
      || overrideStatus.overrideSource === ContentSettingSource.kUser
      || overrideStatus.overrideSource === ContentSettingSource.kNone
    ) {
      return false
    }
    return true
  })

  const enforcedDescription = () => {
    switch (overrideSource) {
      case ContentSettingSource.kExtension:
        return getString(
          'BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN_BY_EXTENSION',
        )
      case ContentSettingSource.kPolicy:
        return getString('BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN_BY_POLICY')
      case ContentSettingSource.kAllowList:
        return getString(
          'BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN_BY_ALLOWLIST',
        )
      case ContentSettingSource.kSupervised:
        return getString(
          'BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN_BY_SUPERVISOR',
        )
      case ContentSettingSource.kInstalledWebApp:
        return getString('BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN_BY_PWA')
      default:
        return getString('BRAVE_SHIELDS_SCRIPTS_BLOCKED_OVERRIDDEN')
    }
  }

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
            <div className='note'>{enforcedDescription()}</div>
          )}
        </div>
        <Toggle
          checked={isNoscriptEnabled}
          disabled={isManaged || scriptsBlockedEnforced}
          size='small'
          onChange={(event) => {
            actions.setIsNoScriptsEnabled(event.checked)
          }}
        />
      </div>
      <button
        onClick={props.showDetails}
        disabled={
          !isNoscriptEnabled || !hasBlockedOrAllowed || scriptsBlockedEnforced
        }
      >
        <Icon name='carat-right' />
      </button>
    </div>
  )
}

function FingerprintingControls(props: { showDetails: () => void }) {
  const actions = useShieldsActions()
  const showStrictMode = useShieldsState((s) => s.showStrictFingerprintingMode)
  const webcompatExceptionsEnabled = useShieldsState(
    (s) => s.isWebcompatExceptionsServiceEnabled,
  )
  const isManaged = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsManaged,
  )
  const fingerprintMode = useShieldsState((s) => s.siteSettings.fingerprintMode)
  const invokedWebcompatCount = useShieldsState(
    (s) => s.siteBlockInfo.invokedWebcompatList.length,
  )

  return (
    <div>
      <Icon name='biometric-login' />
      {showStrictMode ? (
        <select
          value={fingerprintMode}
          disabled={isManaged}
          onChange={(event) => {
            actions.setFingerprintMode(parseInt(event.currentTarget.value))
          }}
        >
          <SelectButton />
          <option value={FingerprintMode.STRICT_MODE}>
            {getString('BRAVE_SHIELDS_FINGERPRINTING_BLOCKED_AGG')}
          </option>
          <option value={FingerprintMode.STANDARD_MODE}>
            {getString('BRAVE_SHIELDS_FINGERPRINTING_BLOCKED_STD')}
          </option>
          <option value={FingerprintMode.ALLOW_MODE}>
            {getString('BRAVE_SHIELDS_FINGERPRINTING_ALLOW_ALL')}
          </option>
        </select>
      ) : (
        <div className='toggle'>
          {getString('BRAVE_SHIELDS_FINGERPRINTING_BLOCKED_STD')}
          <Toggle
            checked={fingerprintMode !== FingerprintMode.ALLOW_MODE}
            disabled={isManaged}
            size='small'
            onChange={(event) => {
              actions.setFingerprintMode(
                event.checked
                  ? FingerprintMode.STANDARD_MODE
                  : FingerprintMode.ALLOW_MODE,
              )
            }}
          />
        </div>
      )}
      <button
        onClick={props.showDetails}
        disabled={
          !webcompatExceptionsEnabled
          || invokedWebcompatCount <= 0
          || fingerprintMode === FingerprintMode.ALLOW_MODE
        }
      >
        <Icon name='carat-right' />
      </button>
    </div>
  )
}

function CookieBlockControls() {
  const actions = useShieldsActions()
  const cookieBlockMode = useShieldsState((s) => s.siteSettings.cookieBlockMode)
  const isManaged = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsManaged,
  )
  return (
    <div>
      <Icon name='cookie' />
      <select
        value={cookieBlockMode}
        disabled={isManaged}
        onChange={(event) => {
          actions.setCookieBlockMode(parseInt(event.currentTarget.value))
        }}
      >
        <SelectButton />
        <option value={CookieBlockMode.BLOCKED}>
          {getString('BRAVE_SHIELDS_COOKIES_BLOCKED')}
        </option>
        <option value={CookieBlockMode.CROSS_SITE_BLOCKED}>
          {getString('BRAVE_SHIELDS_THIRD_PARTY_COOKIES_BLOCKED')}
        </option>
        <option value={CookieBlockMode.ALLOW}>
          {getString('BRAVE_SHIELDS_COOKIES_ALLOWED_ALL')}
        </option>
      </select>
      <div />
    </div>
  )
}

function ForgetFirstPartyStorageControls() {
  const actions = useShieldsActions()
  const forgetFeatureEnabled = useShieldsState(
    (s) => s.isBraveForgetFirstPartyStorageFeatureEnabled,
  )
  const isForgetEnabled = useShieldsState(
    (s) => s.siteSettings.isForgetFirstPartyStorageEnabled,
  )
  const isManaged = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsManaged,
  )

  if (!forgetFeatureEnabled) {
    return null
  }

  return (
    <div>
      <Icon name='eye-off' />
      <div className='toggle'>
        {getString('BRAVE_SHIELDS_FORGET_FIRST_PARTY_STORAGE_LABEL')}
        <Toggle
          checked={isForgetEnabled}
          disabled={isManaged}
          size='small'
          onChange={(event) => {
            actions.setForgetFirstPartyStorageEnabled(event.checked)
          }}
        />
      </div>
      <div />
    </div>
  )
}

function BlockElementsControls() {
  const actions = useShieldsActions()
  const blockedElementsPresent = useShieldsState(
    (s) => s.blockedElementsPresent,
  )

  if (!blockedElementsPresent) {
    return null
  }

  return (
    <div>
      <Icon name='disable-outline' />
      <div className='block-elements'>
        <div>{getString('BRAVE_SHIELDS_BLOCK_ELEMENTS')}</div>
        <button onClick={actions.resetBlockedElements}>
          {getString('BRAVE_SHIELDS_SHOW_ALL_BLOCKED_ELEMENTS')}
        </button>
      </div>
      <div />
    </div>
  )
}

interface CounterProps {
  count: number
}

function Counter(props: CounterProps) {
  if (props.count <= 0) {
    return null
  }
  return (
    <span className='counter'>{props.count > 99 ? '99+' : props.count}</span>
  )
}

interface SelectButtonProps {
  children?: React.ReactNode
}

function SelectButton(props: SelectButtonProps) {
  return (
    <button>
      <selectedcontent></selectedcontent>
      {props.children}
      <Icon name='carat-down' />
    </button>
  )
}

// Remove when <selectedcontent> is a recognized HTML element.
declare global {
  namespace JSX {
    interface IntrinsicElements {
      selectedcontent: React.HTMLAttributes<HTMLElement>
    }
  }
}
