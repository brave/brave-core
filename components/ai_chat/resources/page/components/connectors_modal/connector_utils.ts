// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'

/**
 * The `MCPServerError` that a request to `MCPHost` failed with, or undefined
 * if it failed for another reason, such as the host being unavailable.
 */
export function getMCPServerError(
  error: unknown,
): Mojom.MCPServerError | undefined {
  return typeof error === 'number' ? error : undefined
}

/** The editor field that an `MCPServerError` is about. */
export type ConnectorField = 'name' | 'url' | 'token'

export function getConnectorErrorField(
  error: Mojom.MCPServerError | undefined,
): ConnectorField | undefined {
  switch (error) {
    case Mojom.MCPServerError.kInvalidName:
      return 'name'
    case Mojom.MCPServerError.kInvalidUrl:
    case Mojom.MCPServerError.kUnsupportedScheme:
    case Mojom.MCPServerError.kEmbeddedCredentials:
    case Mojom.MCPServerError.kInsecureScheme:
    case Mojom.MCPServerError.kPrivateAddressNotAllowed:
    case Mojom.MCPServerError.kEndpointTooLong:
      return 'url'
    case Mojom.MCPServerError.kInvalidToken:
      return 'token'
    default:
      return undefined
  }
}

/** What to tell the user about a failed request to `MCPHost`. */
export function getConnectorErrorMessage(error: unknown): string {
  switch (getMCPServerError(error)) {
    case Mojom.MCPServerError.kNotFound:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_NOT_FOUND)
    case Mojom.MCPServerError.kInvalidName:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_INVALID_NAME)
    case Mojom.MCPServerError.kInvalidUrl:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_INVALID_URL)
    case Mojom.MCPServerError.kUnsupportedScheme:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_UNSUPPORTED_SCHEME)
    case Mojom.MCPServerError.kEmbeddedCredentials:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_EMBEDDED_CREDENTIALS)
    case Mojom.MCPServerError.kInsecureScheme:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_INSECURE_SCHEME)
    case Mojom.MCPServerError.kPrivateAddressNotAllowed:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_PRIVATE_ADDRESS_NOT_ALLOWED)
    case Mojom.MCPServerError.kInvalidToken:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_INVALID_TOKEN)
    case Mojom.MCPServerError.kTokenEncryptionFailed:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_TOKEN_ENCRYPTION_FAILED)
    case Mojom.MCPServerError.kEndpointTooLong:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_ENDPOINT_TOO_LONG)
    case Mojom.MCPServerError.kTooManyServers:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_TOO_MANY_SERVERS)
    case Mojom.MCPServerError.kUnsupportedSettingsVersion:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_UNSUPPORTED_SETTINGS_VERSION)
    case Mojom.MCPServerError.kServerDisabled:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_SERVER_DISABLED)
    case Mojom.MCPServerError.kTokenUnreadable:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_TOKEN_UNREADABLE)
    case Mojom.MCPServerError.kServerChanged:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_SERVER_CHANGED)
    case Mojom.MCPServerError.kConnectionFailed:
      return getLocale(S.CHAT_UI_CONNECTOR_ERROR_CONNECTION_FAILED)
    default:
      return getLocale(S.CHAT_UI_ERROR_INTERNAL)
  }
}

// Chromium's `url::kMaxURLChars`.
const kMaxURLChars = 2 * 1024 * 1024

/**
 * The endpoint to send for what the user entered, or the error to show if it
 * can't be sent. The browser closes the connection over a URL that it can't
 * parse, rather than answering with an error, so it must never be sent one.
 */
export function parseConnectorUrl(
  text: string,
): { url: string } | { error: Mojom.MCPServerError } {
  let url: string
  try {
    url = new URL(text.trim()).href
  } catch {
    return { error: Mojom.MCPServerError.kInvalidUrl }
  }
  if (url.length > kMaxURLChars) {
    return { error: Mojom.MCPServerError.kEndpointTooLong }
  }
  return { url }
}

/** The origin a connector's URL is shown as, as its path can hold secrets. */
export function getConnectorOrigin(url: string): string {
  try {
    return new URL(url).origin
  } catch {
    return ''
  }
}

/**
 * The connectors that can be attached to a conversation: the browser only
 * attaches one that's turned on and whose token it can read.
 */
export function getAttachableConnectors(
  servers: Mojom.MCPServer[],
): Mojom.MCPServer[] {
  return servers.filter((server) => server.enabled && !server.tokenUnreadable)
}
