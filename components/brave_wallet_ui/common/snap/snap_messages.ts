// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// postMessage protocol between the trusted wallet page (SnapBridge) and
// chrome-untrusted://snap-executor iframes.
//
// WALLET_PAGE_ORIGIN must track kBraveUIWalletPageURL in
// components/brave_wallet/common/web_ui_constants.h. When the panel is added
// as a frame ancestor this becomes a list. Deliberately not sourced via
// loadTimeData — that would require UseStringsJs() inside the frame that
// evaluates snap code.

export const SNAP_EXECUTOR_ORIGIN = 'chrome-untrusted://snap-executor'
export const WALLET_PAGE_ORIGIN = 'chrome://wallet'

export const enum SnapCommand {
  ExecuteSnap = 'executeSnap',
}

export const enum SnapMessageType {
  ExecutorReady = 'executorReady',
  ExecuteSnapResult = 'executeSnapResult',
}

export interface ExecuteSnapPayload {
  snapId: string
  sourceCode: string
}

export interface ExecuteSnapCommand {
  type: SnapCommand.ExecuteSnap
  requestId: number
  payload: ExecuteSnapPayload
}

export interface ExecuteSnapResult {
  type: SnapMessageType.ExecuteSnapResult
  requestId: number
  success: boolean
  error: string | null
  result: string | null
}

export interface ExecutorReadyMessage {
  type: SnapMessageType.ExecutorReady
}

export type ParentToExecutorMessage = ExecuteSnapCommand
export type ExecutorToParentMessage = ExecuteSnapResult | ExecutorReadyMessage

export function isExecutorReady(data: unknown): data is ExecutorReadyMessage {
  return (
    typeof data === 'object'
    && data !== null
    && (data as ExecutorReadyMessage).type === SnapMessageType.ExecutorReady
  )
}

export function isExecuteSnapResult(data: unknown): data is ExecuteSnapResult {
  return (
    typeof data === 'object'
    && data !== null
    && (data as ExecuteSnapResult).type === SnapMessageType.ExecuteSnapResult
  )
}

export function isExecuteSnapCommand(
  data: unknown,
): data is ExecuteSnapCommand {
  return (
    typeof data === 'object'
    && data !== null
    && (data as ExecuteSnapCommand).type === SnapCommand.ExecuteSnap
    && typeof (data as ExecuteSnapCommand).requestId === 'number'
    && typeof (data as ExecuteSnapCommand).payload === 'object'
  )
}
