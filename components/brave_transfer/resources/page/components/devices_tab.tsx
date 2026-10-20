/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import ProgressBar from '@brave/leo/react/progressBar'
import { formatString } from '$web-common/formatString'

import {
  Device,
  ItemKind,
  Transfer,
  TransferDirection,
  TransferState,
  TrustSource,
} from '../api/transfer_api'
import { useTransferApi } from '../api/transfer_api_context'
import { formatBytes, formatTime } from '../lib/format'
import { getString, StringKey } from '../lib/strings'
import { EmptyState } from './empty_state'
import { deviceIcon, itemKindIcon } from './icons'
import { SendDialog, useSendFiles } from './send_dialog'

import { style } from './devices_tab.style'

// Whether items can be sent to the device right now. Sync-chain devices only
// connect while Sync is on.
export function canSendTo(device: Device, syncEnabled: boolean) {
  return (
    device.isConnected
    && (syncEnabled || device.source !== TrustSource.kSyncChain)
  )
}

function deviceStatus(device: Device, syncEnabled: boolean) {
  if (!syncEnabled && device.source === TrustSource.kSyncChain) {
    return getString('BRAVE_TRANSFER_SYNC_OFF_TITLE')
  }
  if (device.isConnected) {
    return getString('BRAVE_TRANSFER_DEVICE_CONNECTED')
  }
  if (device.lastSeenTime > 0) {
    return formatString(getString('BRAVE_TRANSFER_DEVICE_LAST_SEEN'), [
      formatTime(device.lastSeenTime),
    ])
  }
  return getString('BRAVE_TRANSFER_DEVICE_NOT_NEARBY')
}

interface DeviceRowProps {
  device: Device
  syncEnabled: boolean
  onSend: () => void
}

function DeviceRow(props: DeviceRowProps) {
  const { device } = props
  const api = useTransferApi()
  const isIOS = api.useIsIOSData()
  const sendFiles = useSendFiles()
  const [isDragOver, setIsDragOver] = React.useState(false)
  const [error, setError] = React.useState<StringKey | null>(null)
  const canSend = canSendTo(device, props.syncEnabled)
  const acceptsDrops = canSend && !isIOS

  async function onDrop(event: React.DragEvent) {
    event.preventDefault()
    setIsDragOver(false)
    setError(await sendFiles(device.id, Array.from(event.dataTransfer.files)))
  }

  return (
    <li
      className='device'
      data-connected={canSend}
      data-drag-over={isDragOver}
      onDragOver={
        acceptsDrops
          ? (event) => {
              event.preventDefault()
              setIsDragOver(true)
            }
          : undefined
      }
      onDragLeave={acceptsDrops ? () => setIsDragOver(false) : undefined}
      onDrop={acceptsDrops ? onDrop : undefined}
    >
      <div className='device-icon'>
        <Icon name={deviceIcon(device.type)} />
      </div>
      <div className='device-info'>
        <span className='device-name'>{device.name}</span>
        <span className='device-status'>
          {canSend && <span className='status-dot' />}
          {isDragOver
            ? getString('BRAVE_TRANSFER_DEVICE_DROP_HINT')
            : deviceStatus(device, props.syncEnabled)}
        </span>
        {error && <span className='device-error'>{getString(error)}</span>}
      </div>
      <Button
        kind={canSend ? 'filled' : 'outline'}
        size='small'
        isDisabled={!canSend}
        onClick={props.onSend}
      >
        <Icon
          slot='icon-before'
          name='send'
        />
        {getString('BRAVE_TRANSFER_DEVICE_SEND_BUTTON')}
      </Button>
    </li>
  )
}

function DeviceList() {
  const api = useTransferApi()
  const devices = api.useGetDevicesData()
  const { syncEnabled } = api.useGetServiceStateData()
  const [sendTo, setSendTo] = React.useState<Device | null>(null)

  if (devices.length === 0) {
    return (
      <EmptyState
        icon='smartphone-laptop'
        title={getString('BRAVE_TRANSFER_DEVICES_EMPTY_TITLE')}
        text={getString('BRAVE_TRANSFER_DEVICES_EMPTY_TEXT')}
      />
    )
  }

  // Devices that can be sent to come first.
  const sorted = [...devices].sort(
    (a, b) =>
      Number(canSendTo(b, syncEnabled)) - Number(canSendTo(a, syncEnabled)),
  )

  return (
    <>
      <ul className='devices'>
        {sorted.map((device) => (
          <DeviceRow
            key={device.id}
            device={device}
            syncEnabled={syncEnabled}
            onSend={() => setSendTo(device)}
          />
        ))}
      </ul>
      {sendTo && (
        <SendDialog
          device={sendTo}
          onClose={() => setSendTo(null)}
        />
      )}
    </>
  )
}

function transferStateText(transfer: Transfer) {
  const strings: Record<number, StringKey> = {
    [TransferState.kWaitingForResponse]: 'BRAVE_TRANSFER_STATE_WAITING',
    [TransferState.kCompleted]: 'BRAVE_TRANSFER_STATE_COMPLETED',
    [TransferState.kDeclined]: 'BRAVE_TRANSFER_STATE_DECLINED',
    [TransferState.kExpired]: 'BRAVE_TRANSFER_STATE_EXPIRED',
    [TransferState.kCancelled]: 'BRAVE_TRANSFER_STATE_CANCELLED',
    [TransferState.kFailed]: 'BRAVE_TRANSFER_STATE_FAILED',
  }
  if (transfer.state === TransferState.kInProgress) {
    return formatString(getString('BRAVE_TRANSFER_STATE_PROGRESS'), [
      formatBytes(transfer.bytesTransferred),
      formatBytes(transfer.bytesTotal),
    ])
  }
  return getString(strings[transfer.state])
}

function transferSummary(transfer: Transfer) {
  const [first, ...rest] = transfer.items
  if (!first) {
    return ''
  }
  const isFile = first.kind === ItemKind.kImage || first.kind === ItemKind.kFile
  const name = isFile ? first.name : first.text
  return rest.length > 0 ? `${name} +${rest.length}` : name
}

function TransferRow(props: { transfer: Transfer }) {
  const api = useTransferApi()
  const { transfer } = props
  const isActive =
    transfer.state === TransferState.kWaitingForResponse
    || transfer.state === TransferState.kInProgress
  const progress =
    transfer.bytesTotal > BigInt(0)
      ? Number(transfer.bytesTransferred) / Number(transfer.bytesTotal)
      : 0

  return (
    <li
      className='transfer'
      data-state={transfer.state}
    >
      <Icon
        className='transfer-icon'
        name={itemKindIcon(transfer.items[0]?.kind ?? ItemKind.kFile)}
      />
      <div className='transfer-info'>
        <span className='transfer-summary'>{transferSummary(transfer)}</span>
        <span className='transfer-meta'>
          {formatString(
            getString(
              transfer.direction === TransferDirection.kSend
                ? 'BRAVE_TRANSFER_TRANSFER_TO'
                : 'BRAVE_TRANSFER_TRANSFER_FROM',
            ),
            [transfer.deviceName],
          )}
          {' · '}
          {transferStateText(transfer)}
        </span>
        {transfer.state === TransferState.kInProgress && (
          <ProgressBar progress={progress} />
        )}
      </div>
      {isActive && (
        <Button
          kind='plain-faint'
          size='small'
          fab
          title={getString('BRAVE_TRANSFER_TRANSFER_CANCEL')}
          onClick={() => api.cancelTransfer(transfer.id)}
        >
          <Icon name='close' />
        </Button>
      )}
    </li>
  )
}

function TransferList() {
  const api = useTransferApi()
  const transfers = api.useGetTransfersData()
  if (transfers.length === 0) {
    return null
  }
  return (
    <section className='transfers'>
      <h2>{getString('BRAVE_TRANSFER_TRANSFERS_TITLE')}</h2>
      <ul>
        {transfers.map((transfer) => (
          <TransferRow
            key={transfer.id}
            transfer={transfer}
          />
        ))}
      </ul>
    </section>
  )
}

export function DevicesTab() {
  return (
    <div data-css-scope={style.scope}>
      <section>
        <h2>{getString('BRAVE_TRANSFER_DEVICES_TITLE')}</h2>
        <DeviceList />
      </section>
      <TransferList />
    </div>
  )
}
