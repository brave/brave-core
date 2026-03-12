// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// postMessage protocol between the wallet page (SnapBridge) and
// chrome-untrusted://snap-executor/ iframes.

// ---------------------------------------------------------------------------
// Wallet page → iframe
// ---------------------------------------------------------------------------

/** Tell the iframe to load and initialize a snap. */
export type LoadSnapCommand = {
  type: 'load_snap'
  snapId: string
  /** Bundled snap source code (CommonJS). */
  source: string
  requestId: string
}

/** Invoke an RPC method on a loaded snap. */
export type InvokeSnapCommand = {
  type: 'invoke'
  method: string
  params: unknown
  /** The origin of the dapp that triggered the call. */
  origin: string
  requestId: string
}

/** Deliver a snap.request() response from C++ core back to the iframe. */
export type SnapRequestResponse = {
  type: 'snap_request_response'
  requestId: string
  result?: unknown
  error?: string
}

/** Union of all messages the wallet page sends to an iframe. */
export type ParentToIframeMessage =
  | LoadSnapCommand
  | InvokeSnapCommand
  | SnapRequestResponse

// ---------------------------------------------------------------------------
// Iframe → wallet page
// ---------------------------------------------------------------------------

/** Response to a LoadSnapCommand or InvokeSnapCommand. */
export type SnapResponse = {
  type: 'response'
  requestId: string
  result?: unknown
  error?: string
}

/** A snap.request() call from the snap code, relayed to C++ core. */
export type SnapRequestMessage = {
  type: 'snap_request'
  snapId: string
  method: string
  params: unknown
  requestId: string
}

/** Union of all messages an iframe sends to the wallet page. */
export type IframeToParentMessage = SnapResponse | SnapRequestMessage

// ---------------------------------------------------------------------------
// Type guards
// ---------------------------------------------------------------------------

function isObject(v: unknown): v is Record<string, unknown> {
  return typeof v === 'object' && v !== null
}

export function isSnapResponse(msg: unknown): msg is SnapResponse {
  return isObject(msg) && msg['type'] === 'response'
}

export function isSnapRequestMessage(
  msg: unknown,
): msg is SnapRequestMessage {
  return isObject(msg) && msg['type'] === 'snap_request'
}

// ---------------------------------------------------------------------------
// Factories
// ---------------------------------------------------------------------------

export function makeLoadSnapCommand(
  snapId: string,
  source: string,
  requestId: string,
): LoadSnapCommand {
  return { type: 'load_snap', snapId, source, requestId }
}

export function makeInvokeSnapCommand(
  method: string,
  params: unknown,
  origin: string,
  requestId: string,
): InvokeSnapCommand {
  return { type: 'invoke', method, params, origin, requestId }
}

export function makeSnapRequestResponse(
  requestId: string,
  result?: unknown,
  error?: string,
): SnapRequestResponse {
  return { type: 'snap_request_response', requestId, result, error }
}
