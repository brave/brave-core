/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dropdown from '@brave/leo/react/dropdown'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'
import { formatString } from '$web-common/formatString'

import {
  Device,
  DevicePolicy,
  ReceiveMode,
  Settings,
  TrustSource,
} from '../api/transfer_api'
import { useTransferApi } from '../api/transfer_api_context'
import { formatTime } from '../lib/format'
import { getString, StringKey } from '../lib/strings'
import { deviceIcon } from './icons'
import { PermissionsCard } from './notices'

import { style } from './settings_tab.style'

const receiveModeStrings: Record<ReceiveMode, StringKey> = {
  [ReceiveMode.kOff]: 'BRAVE_TRANSFER_SETTINGS_RECEIVE_OFF',
  [ReceiveMode.kTrustedDevices]: 'BRAVE_TRANSFER_SETTINGS_RECEIVE_TRUSTED',
}

const policyStrings: Record<DevicePolicy, StringKey> = {
  [DevicePolicy.kDefault]: 'BRAVE_TRANSFER_POLICY_DEFAULT',
  [DevicePolicy.kAlwaysAccept]: 'BRAVE_TRANSFER_POLICY_ALWAYS_ACCEPT',
  [DevicePolicy.kAlwaysAsk]: 'BRAVE_TRANSFER_POLICY_ALWAYS_ASK',
  [DevicePolicy.kBlock]: 'BRAVE_TRANSFER_POLICY_BLOCK',
}

const policies = [
  DevicePolicy.kDefault,
  DevicePolicy.kAlwaysAccept,
  DevicePolicy.kAlwaysAsk,
  DevicePolicy.kBlock,
]

interface ToggleRowProps {
  label: string
  description?: string
  checked: boolean
  disabled?: boolean
  onChange: (checked: boolean) => void
}

function ToggleRow(props: ToggleRowProps) {
  return (
    <div className='setting'>
      <div className='setting-text'>
        <span className='setting-label'>{props.label}</span>
        {props.description && (
          <span className='setting-description'>{props.description}</span>
        )}
      </div>
      <Toggle
        size='small'
        checked={props.checked}
        disabled={props.disabled}
        aria-label={props.label}
        onChange={({ checked }) => props.onChange(checked)}
      />
    </div>
  )
}

function SaveLocation(props: { settings: Settings }) {
  const api = useTransferApi()
  const isIOS = api.useIsIOSData()
  const { settings } = props

  if (isIOS) {
    return (
      <div className='setting save-location'>
        <div className='setting-text'>
          <span className='setting-label'>
            {getString('BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION')}
          </span>
          <span className='setting-description'>
            {getString('BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION_IOS_FILES')}
          </span>
          <span className='setting-description'>
            {getString('BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION_IOS_PHOTOS')}
          </span>
        </div>
      </div>
    )
  }

  const usesDownloads = !settings.saveDirectory
  return (
    <div className='setting save-location'>
      <div className='setting-text'>
        <span className='setting-label'>
          {getString('BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION')}
        </span>
        <span className='setting-description path'>
          <Icon name='folder' />
          {usesDownloads
            ? `${getString(
                'BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION_DEFAULT',
              )} (${settings.defaultSaveDirectory})`
            : settings.saveDirectory}
        </span>
      </div>
      <div className='setting-actions'>
        {!usesDownloads && (
          <Button
            kind='plain-faint'
            size='small'
            onClick={() =>
              api.updateSettings([{ ...settings, saveDirectory: '' }])
            }
          >
            {getString('BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION_RESET')}
          </Button>
        )}
        <Button
          kind='outline'
          size='small'
          onClick={() => api.chooseSaveDirectory()}
        >
          {getString('BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION_CHANGE')}
        </Button>
      </div>
    </div>
  )
}

function TrustedDeviceRow(props: { device: Device }) {
  const api = useTransferApi()
  const { device } = props
  const details = [
    getString(
      device.source === TrustSource.kSyncChain
        ? 'BRAVE_TRANSFER_SOURCE_SYNC'
        : 'BRAVE_TRANSFER_SOURCE_PAIRED',
    ),
    device.lastSeenTime > 0
      ? formatString(getString('BRAVE_TRANSFER_DEVICE_LAST_SEEN'), [
          formatTime(device.lastSeenTime),
        ])
      : getString('BRAVE_TRANSFER_NEVER_CONNECTED'),
  ]

  return (
    <li className='trusted-device'>
      <Icon
        className='trusted-device-icon'
        name={deviceIcon(device.type)}
      />
      <div className='trusted-device-info'>
        <span className='setting-label'>{device.name}</span>
        <span className='setting-description'>{details.join(' · ')}</span>
        {device.keyFingerprint && (
          <span className='fingerprint'>
            {formatString(getString('BRAVE_TRANSFER_KEY_FINGERPRINT'), [
              device.keyFingerprint,
            ])}
          </span>
        )}
      </div>
      <div className='trusted-device-actions'>
        <Dropdown
          size='small'
          value={String(device.policy)}
          positionStrategy='fixed'
          aria-label={getString('BRAVE_TRANSFER_DEVICE_POLICY_LABEL')}
          onChange={({ value }) =>
            api.setDevicePolicy([device.id, Number(value) as DevicePolicy])
          }
        >
          <span slot='value'>{getString(policyStrings[device.policy])}</span>
          {policies.map((policy) => (
            <leo-option
              key={policy}
              value={String(policy)}
            >
              {getString(policyStrings[policy])}
            </leo-option>
          ))}
        </Dropdown>
        <Button
          kind='plain-faint'
          size='small'
          onClick={() => api.removeDevice([device.id])}
        >
          {getString('BRAVE_TRANSFER_REMOVE_DEVICE')}
        </Button>
      </div>
    </li>
  )
}

export function SettingsTab() {
  const api = useTransferApi()
  const isIOS = api.useIsIOSData()
  const settings = api.useGetSettings().data
  const devices = api.useGetDevicesData()
  const { syncEnabled } = api.useGetServiceStateData()

  if (!settings) {
    return null
  }

  const update = (changes: Partial<Settings>) =>
    api.updateSettings([{ ...settings, ...changes }])
  const receiveOff = settings.receiveMode === ReceiveMode.kOff

  return (
    <div data-css-scope={style.scope}>
      {isIOS && <PermissionsCard alwaysShow />}
      <section>
        <div className='setting'>
          <div className='setting-text'>
            <span className='setting-label'>
              {getString('BRAVE_TRANSFER_SETTINGS_RECEIVE')}
            </span>
          </div>
          <Dropdown
            size='small'
            value={String(settings.receiveMode)}
            positionStrategy='fixed'
            aria-label={getString('BRAVE_TRANSFER_SETTINGS_RECEIVE')}
            onChange={({ value }) =>
              update({ receiveMode: Number(value) as ReceiveMode })
            }
          >
            <span slot='value'>
              {getString(receiveModeStrings[settings.receiveMode])}
            </span>
            <leo-option value={String(ReceiveMode.kOff)}>
              {getString(receiveModeStrings[ReceiveMode.kOff])}
            </leo-option>
            <leo-option value={String(ReceiveMode.kTrustedDevices)}>
              {getString(receiveModeStrings[ReceiveMode.kTrustedDevices])}
            </leo-option>
          </Dropdown>
        </div>
        <ToggleRow
          label={getString('BRAVE_TRANSFER_SETTINGS_AUTO_ACCEPT')}
          description={getString('BRAVE_TRANSFER_SETTINGS_AUTO_ACCEPT_DESC')}
          checked={settings.autoAcceptTrusted}
          disabled={receiveOff}
          onChange={(autoAcceptTrusted) => update({ autoAcceptTrusted })}
        />
        <ToggleRow
          label={getString('BRAVE_TRANSFER_SETTINGS_AUTO_TRUST')}
          description={getString(
            syncEnabled
              ? 'BRAVE_TRANSFER_SETTINGS_AUTO_TRUST_DESC'
              : 'BRAVE_TRANSFER_SETTINGS_AUTO_TRUST_SYNC_OFF',
          )}
          checked={settings.autoTrustSyncChain}
          onChange={(autoTrustSyncChain) => update({ autoTrustSyncChain })}
        />
        <ToggleRow
          label={getString('BRAVE_TRANSFER_SETTINGS_NOTIFY')}
          checked={settings.notifyOnReceive}
          disabled={receiveOff}
          onChange={(notifyOnReceive) => update({ notifyOnReceive })}
        />
        <SaveLocation settings={settings} />
      </section>
      <section>
        <h2>{getString('BRAVE_TRANSFER_SETTINGS_TRUSTED_DEVICES')}</h2>
        {devices.length === 0 ? (
          <p className='setting-description'>
            {getString('BRAVE_TRANSFER_SETTINGS_TRUSTED_DEVICES_EMPTY')}
          </p>
        ) : (
          <ul>
            {devices.map((device) => (
              <TrustedDeviceRow
                key={device.id}
                device={device}
              />
            ))}
          </ul>
        )}
      </section>
    </div>
  )
}
