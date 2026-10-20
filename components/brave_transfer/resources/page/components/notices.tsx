/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { PermissionStatus } from '../api/transfer_api'
import { useTransferApi } from '../api/transfer_api_context'
import { getString, StringKey } from '../lib/strings'

import { style } from './notices.style'

export function SyncOffNotice() {
  const api = useTransferApi()
  return (
    <div data-css-scope={style.scope}>
      <Alert type='warning'>
        <Icon
          slot='icon'
          name='product-sync'
        />
        <div slot='title'>{getString('BRAVE_TRANSFER_SYNC_OFF_TITLE')}</div>
        {getString('BRAVE_TRANSFER_SYNC_OFF_TEXT')}
        <div slot='actions'>
          <Button
            kind='filled'
            size='small'
            onClick={() => api.openSyncSettings()}
          >
            {getString('BRAVE_TRANSFER_SYNC_OFF_BUTTON')}
          </Button>
        </div>
      </Alert>
    </div>
  )
}

function permissionStatusText(status: PermissionStatus): StringKey {
  switch (status) {
    case PermissionStatus.kGranted:
      return 'BRAVE_TRANSFER_PERMISSION_GRANTED'
    case PermissionStatus.kLimited:
      return 'BRAVE_TRANSFER_PERMISSION_LIMITED'
    case PermissionStatus.kDenied:
      return 'BRAVE_TRANSFER_PERMISSION_DENIED'
    default:
      return 'BRAVE_TRANSFER_PERMISSION_NOT_DETERMINED'
  }
}

function isAllowed(status: PermissionStatus) {
  return (
    status === PermissionStatus.kGranted || status === PermissionStatus.kLimited
  )
}

interface PermissionsCardProps {
  // Show the card even when every permission is allowed.
  alwaysShow?: boolean
}

// iOS only: the Local Network and Photos (add-only) permissions.
export function PermissionsCard(props: PermissionsCardProps) {
  const api = useTransferApi()
  const permissions = api.useGetPermissions().data
  const requestPermissions = api.useRequestPermissions()

  if (!permissions) {
    return null
  }

  const rows = [
    {
      key: 'local-network',
      label: getString('BRAVE_TRANSFER_PERMISSION_LOCAL_NETWORK'),
      icon: 'wifi',
      status: permissions.localNetwork,
    },
    {
      key: 'photos',
      label: getString('BRAVE_TRANSFER_PERMISSION_PHOTOS'),
      icon: 'photos',
      status: permissions.photos,
    },
  ]

  const allAllowed = rows.every((row) => isAllowed(row.status))
  if (allAllowed && !props.alwaysShow) {
    return null
  }
  const anyDenied = rows.some((row) => row.status === PermissionStatus.kDenied)
  const anyUnasked = rows.some(
    (row) => row.status === PermissionStatus.kNotDetermined,
  )

  return (
    <section
      data-css-scope={style.scope}
      className='permissions'
    >
      <h3>{getString('BRAVE_TRANSFER_PERMISSIONS_TITLE')}</h3>
      <p className='permissions-text'>
        {getString('BRAVE_TRANSFER_PERMISSIONS_TEXT')}
      </p>
      <ul>
        {rows.map((row) => (
          <li key={row.key}>
            <Icon name={row.icon} />
            <span className='permission-name'>{row.label}</span>
            <span
              className='permission-status'
              data-allowed={isAllowed(row.status)}
            >
              {getString(permissionStatusText(row.status))}
            </span>
          </li>
        ))}
      </ul>
      {!allAllowed && (
        <div className='permission-actions'>
          {anyUnasked && (
            <Button
              size='small'
              isLoading={requestPermissions.isPending}
              onClick={() => requestPermissions.mutate([])}
            >
              {getString('BRAVE_TRANSFER_PERMISSIONS_ALLOW')}
            </Button>
          )}
          {anyDenied && (
            <Button
              size='small'
              kind='outline'
              onClick={() => api.openSystemSettings()}
            >
              {getString('BRAVE_TRANSFER_PERMISSIONS_OPEN_SETTINGS')}
            </Button>
          )}
        </div>
      )}
    </section>
  )
}
