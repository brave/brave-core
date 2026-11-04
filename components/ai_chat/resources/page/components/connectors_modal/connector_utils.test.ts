// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import {
  canReconnectConnector,
  getAttachableConnectors,
  getConnectorNoticeText,
  getConnectorStatusDescription,
  getConnectorStatusLabel,
  isConnectorFailing,
  getConnectorErrorField,
  getConnectorErrorMessage,
  getConnectorOrigin,
  getMCPServerError,
  parseConnectorUrl,
} from './connector_utils'

function allErrors(): Mojom.MCPServerError[] {
  const errors: Mojom.MCPServerError[] = []
  for (
    let error = Mojom.MCPServerError.MIN_VALUE;
    error <= Mojom.MCPServerError.MAX_VALUE;
    ++error
  ) {
    errors.push(error)
  }
  return errors
}

describe('getConnectorErrorMessage', () => {
  it('gives every MCPServerError its own message', () => {
    const messages = allErrors().map(getConnectorErrorMessage)
    for (const message of messages) {
      expect(message).toMatch(/^CHAT_UI_CONNECTOR_ERROR_/)
    }
    expect(new Set(messages).size).toBe(messages.length)
  })

  it('falls back to a general message when the host is unavailable', () => {
    const error = new Error('The pipe has already been closed.')
    expect(getMCPServerError(error)).toBeUndefined()
    expect(getConnectorErrorMessage(error)).toBe('CHAT_UI_ERROR_INTERNAL')
  })
})

describe('getConnectorErrorField', () => {
  it('puts each validation error at the field it is about', () => {
    expect(getConnectorErrorField(Mojom.MCPServerError.kInvalidName)).toBe(
      'name',
    )
    for (const error of [
      Mojom.MCPServerError.kInvalidUrl,
      Mojom.MCPServerError.kUnsupportedScheme,
      Mojom.MCPServerError.kEmbeddedCredentials,
      Mojom.MCPServerError.kInsecureScheme,
      Mojom.MCPServerError.kPrivateAddressNotAllowed,
      Mojom.MCPServerError.kEndpointTooLong,
    ]) {
      expect(getConnectorErrorField(error)).toBe('url')
    }
    expect(getConnectorErrorField(Mojom.MCPServerError.kInvalidToken)).toBe(
      'token',
    )
    expect(
      getConnectorErrorField(Mojom.MCPServerError.kTooManyServers),
    ).toBeUndefined()
    expect(getConnectorErrorField(undefined)).toBeUndefined()
  })
})

describe('parseConnectorUrl', () => {
  it('accepts a full URL, ignoring surrounding whitespace', () => {
    expect(parseConnectorUrl('  https://mcp.example.com/mcp ')).toEqual({
      url: 'https://mcp.example.com/mcp',
    })
  })

  // Sending a URL the browser can't parse would close the connection.
  it('rejects text that is not a URL, rather than sending it', () => {
    expect(parseConnectorUrl('mcp.example.com/mcp')).toEqual({
      error: Mojom.MCPServerError.kInvalidUrl,
    })
    expect(parseConnectorUrl('')).toEqual({
      error: Mojom.MCPServerError.kInvalidUrl,
    })
  })

  it('rejects a URL longer than the browser accepts', () => {
    const url = `https://example.com/${'a'.repeat(2 * 1024 * 1024)}`
    expect(parseConnectorUrl(url)).toEqual({
      error: Mojom.MCPServerError.kEndpointTooLong,
    })
  })

  // A scheme the browser rejects is still a URL, so it gets the browser's
  // own error.
  it('leaves other checks to the browser', () => {
    expect(parseConnectorUrl('ftp://example.com/mcp')).toEqual({
      url: 'ftp://example.com/mcp',
    })
  })
})

describe('getConnectorOrigin', () => {
  it('drops the path, which can hold secrets', () => {
    expect(getConnectorOrigin('https://mcp.example.com/key/abc123/mcp')).toBe(
      'https://mcp.example.com',
    )
    expect(getConnectorOrigin('not a url')).toBe('')
  })
})

describe('getAttachableConnectors', () => {
  const server = (
    id: string,
    overrides: Partial<Mojom.MCPServer> = {},
  ): Mojom.MCPServer => ({
    id,
    name: id,
    endpoint: { url: `https://${id}.example.com/mcp` },
    enabled: true,
    autoAttach: false,
    slug: id,
    hasToken: false,
    tokenUnreadable: false,
    ...overrides,
  })

  it('leaves out connectors that are off, or whose token is unreadable', () => {
    const servers = [
      server('on'),
      server('off', { enabled: false }),
      server('unreadable', { hasToken: true, tokenUnreadable: true }),
      server('with-token', { hasToken: true }),
    ]
    expect(getAttachableConnectors(servers).map((s) => s.id)).toEqual([
      'on',
      'with-token',
    ])
  })
})

describe('connector status', () => {
  const Status = Mojom.MCPServerStatus

  it('says which statuses are failing, and which reconnecting could change', () => {
    expect(isConnectorFailing(Status.kAvailable)).toBe(false)
    expect(isConnectorFailing(Status.kChecking)).toBe(false)
    for (const status of [
      Status.kNotResponding,
      Status.kAuthRequired,
      Status.kUnsupported,
      Status.kUnavailable,
    ]) {
      expect(isConnectorFailing(status)).toBe(true)
    }
    expect(canReconnectConnector(Status.kNotResponding)).toBe(true)
    expect(canReconnectConnector(Status.kUnsupported)).toBe(true)
    expect(canReconnectConnector(Status.kAuthRequired)).toBe(false)
    expect(canReconnectConnector(Status.kUnavailable)).toBe(false)
    expect(canReconnectConnector(Status.kChecking)).toBe(false)
  })

  it('describes each status', () => {
    expect(getConnectorStatusLabel(Status.kAvailable)).toBe(
      'CHAT_UI_MCP_SERVER_LABEL',
    )
    expect(getConnectorStatusLabel(Status.kChecking)).toBe(
      'CHAT_UI_CONNECTOR_STATUS_CONNECTING',
    )
    expect(getConnectorStatusDescription(Status.kAvailable)).toBeUndefined()
    expect(getConnectorStatusDescription(Status.kChecking)).toBeUndefined()
    for (const [status, name] of [
      [Status.kNotResponding, 'NOT_RESPONDING'],
      [Status.kAuthRequired, 'AUTH_REQUIRED'],
      [Status.kUnsupported, 'UNSUPPORTED'],
      [Status.kUnavailable, 'UNAVAILABLE'],
    ] as const) {
      expect(getConnectorStatusLabel(status)).toBe(
        `CHAT_UI_CONNECTOR_STATUS_${name}`,
      )
      expect(getConnectorStatusDescription(status)).toBe(
        `CHAT_UI_CONNECTOR_STATUS_${name}_DESCRIPTION`,
      )
    }
  })

  it('words the notice for an auth error with the token', () => {
    expect(getConnectorNoticeText(Status.kNotResponding, 'Server')).toBe(
      'CHAT_UI_CONNECTOR_NOTICE_NOT_RESPONDING',
    )
    expect(getConnectorNoticeText(Status.kAuthRequired, 'Server')).toBe(
      'CHAT_UI_CONNECTOR_NOTICE_AUTH_REQUIRED',
    )
    expect(getConnectorNoticeText(Status.kUnsupported, 'Server')).toBe(
      'CHAT_UI_CONNECTOR_NOTICE_UNAVAILABLE',
    )
    expect(getConnectorNoticeText(Status.kUnavailable, 'Server')).toBe(
      'CHAT_UI_CONNECTOR_NOTICE_UNAVAILABLE',
    )
  })
})
