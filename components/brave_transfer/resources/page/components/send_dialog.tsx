/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import Textarea from '@brave/leo/react/textarea'
import { formatString } from '$web-common/formatString'

import { Device, SendResult } from '../api/transfer_api'
import { useTransferApi } from '../api/transfer_api_context'
import { filesFitInMessage, readFiles } from '../lib/read_files'
import { getString, StringKey } from '../lib/strings'

import { style } from './send_dialog.style'

export function sendErrorText(result: SendResult): StringKey | null {
  switch (result) {
    case SendResult.kSuccess:
    case SendResult.kCancelled:
      return null
    case SendResult.kDeviceUnavailable:
      return 'BRAVE_TRANSFER_SEND_ERROR_UNAVAILABLE'
    case SendResult.kTooLarge:
      return 'BRAVE_TRANSFER_SEND_ERROR_TOO_LARGE'
    default:
      return 'BRAVE_TRANSFER_SEND_ERROR_FAILED'
  }
}

// Sends files from a drop or a file input, and returns the error to show, if
// any.
export function useSendFiles() {
  const api = useTransferApi()
  return React.useCallback(
    async (deviceId: string, files: File[]): Promise<StringKey | null> => {
      if (files.length === 0) {
        return null
      }
      if (!filesFitInMessage(files)) {
        return 'BRAVE_TRANSFER_SEND_ERROR_TOO_LARGE'
      }
      const result = await api.sendFiles([deviceId, await readFiles(files)])
      return sendErrorText(result)
    },
    [api],
  )
}

interface Props {
  device: Device
  onClose: () => void
}

export function SendDialog(props: Props) {
  const api = useTransferApi()
  const isIOS = api.useIsIOSData()
  const sendFiles = useSendFiles()
  const fileInput = React.useRef<HTMLInputElement>(null)
  const [text, setText] = React.useState('')
  const [error, setError] = React.useState<StringKey | null>(null)
  const [isSending, setIsSending] = React.useState(false)
  const [isDragOver, setIsDragOver] = React.useState(false)

  const deviceId = props.device.id

  async function run(send: () => Promise<StringKey | null>) {
    setIsSending(true)
    setError(null)
    const sendError = await send()
    setIsSending(false)
    if (sendError) {
      setError(sendError)
    } else {
      props.onClose()
    }
  }

  function onSendText() {
    if (!text.trim()) {
      return
    }
    run(async () => sendErrorText(await api.sendText([deviceId, text])))
  }

  function onPickAndSend() {
    run(async () => {
      const result = await api.pickAndSend([deviceId])
      return result === SendResult.kCancelled ? null : sendErrorText(result)
    })
  }

  function onFilesChosen(files: FileList | null) {
    run(() => sendFiles(deviceId, Array.from(files ?? [])))
  }

  function onDrop(event: React.DragEvent) {
    event.preventDefault()
    setIsDragOver(false)
    onFilesChosen(event.dataTransfer.files)
  }

  return (
    <Dialog
      isOpen
      showClose
      onClose={props.onClose}
    >
      <div slot='title'>
        {formatString(getString('BRAVE_TRANSFER_SEND_TITLE'), [
          props.device.name,
        ])}
      </div>
      <div
        data-css-scope={style.scope}
        className='send'
      >
        <Textarea
          value={text}
          minRows={3}
          placeholder={getString('BRAVE_TRANSFER_SEND_TEXT_PLACEHOLDER')}
          onInput={(detail) => setText(detail.value)}
        >
          {getString('BRAVE_TRANSFER_SEND_TEXT_LABEL')}
        </Textarea>
        <div className='text-actions'>
          <Button
            isDisabled={!text.trim() || isSending}
            onClick={onSendText}
          >
            {getString('BRAVE_TRANSFER_SEND_TEXT_BUTTON')}
          </Button>
        </div>
        {isIOS ? (
          <Button
            kind='outline'
            isDisabled={isSending}
            onClick={onPickAndSend}
          >
            <Icon
              slot='icon-before'
              name='photos'
            />
            {getString('BRAVE_TRANSFER_SEND_CHOOSE_PHOTOS_OR_FILES')}
          </Button>
        ) : (
          <div
            className='drop-zone'
            data-drag-over={isDragOver}
            onDragOver={(event) => {
              event.preventDefault()
              setIsDragOver(true)
            }}
            onDragLeave={() => setIsDragOver(false)}
            onDrop={onDrop}
          >
            <Icon name='upload' />
            <span>{getString('BRAVE_TRANSFER_SEND_DROP_ZONE')}</span>
            <Button
              kind='outline'
              size='small'
              isDisabled={isSending}
              onClick={() => fileInput.current?.click()}
            >
              {getString('BRAVE_TRANSFER_SEND_CHOOSE_FILES')}
            </Button>
            <input
              ref={fileInput}
              type='file'
              multiple
              hidden
              data-testid='file-input'
              onChange={(event) => onFilesChosen(event.target.files)}
            />
          </div>
        )}
        {error && <Alert type='error'>{getString(error)}</Alert>}
      </div>
    </Dialog>
  )
}
