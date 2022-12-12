/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from '../../shared/lib/mojom'

export type ExternalWalletProvider =
  'uphold' |
  'bitflyer' |
  'gemini'

export interface ExternalWallet {
  provider: ExternalWalletProvider
  status: mojom.WalletStatus
  username: string
  links: {
    account?: string
    reconnect?: string
  }
}

// Returns the external wallet provider name for the specified provider.
export function getExternalWalletProviderName (
  provider: ExternalWalletProvider
) {
  switch (provider) {
    case 'bitflyer': return 'bitFlyer'
    case 'gemini': return 'Gemini'
    case 'uphold': return 'Uphold'
  }
}

// Returns an |ExternalWalletProvider| matching the given provider key, or
// |null| if the key is invalid. This function is provided for backward
// compatibility with code that does not yet use the |ExternalWalletProvider|
// type.
export function externalWalletProviderFromString (
  key: string
): ExternalWalletProvider | null {
  switch (key) {
    case 'bitflyer':
    case 'gemini':
    case 'uphold':
      return key
    default:
      return null
  }
}

// Returns the external wallet provider name associated with the specified
// provider key, or the empty string if the key is not recognized. This function
// is provided for backward compatibility with code that does not yet use the
// |ExternalWalletProvider| type. Prefer |getExternalWalletProviderName|.
export function lookupExternalWalletProviderName (providerKey: string) {
  const provider = externalWalletProviderFromString(providerKey)
  return provider ? getExternalWalletProviderName(provider) : ''
}

// Converts external wallet information returned from the `chrome.braveRewards`
// extension API into an |ExternalWallet| object, or |null| if the specified
// object cannot be converted.
export function externalWalletFromExtensionData (
  data: any
): ExternalWallet | null {
  function mapStatus (status: number): mojom.WalletStatus | null {
    switch (status) {
      case 2:
        return mojom.WalletStatus.kConnected
      case 4:
        return mojom.WalletStatus.kLoggedOut
    }
    return null
  }

  if (!data || typeof data !== 'object') {
    return null
  }

  const provider = externalWalletProviderFromString(String(data.type || ''))
  const status = mapStatus(Number(data.status || 0))

  if (!provider || !status) {
    return null
  }

  return {
    provider,
    status,
    username: String(data.userName || ''),
    links: {
      account: String(data.accountUrl || ''),
      reconnect: String(data.loginUrl || '')
    }
  }
}

export interface ExternalWalletProviderRegionInfo {
  allow: string[]
  block: string[]
}

export function isExternalWalletProviderAllowed (
  countryCode: string,
  regionInfo: ExternalWalletProviderRegionInfo | null
) {
  if (!regionInfo) {
    regionInfo = { allow: [], block: [] }
  }
  const { allow, block } = regionInfo
  if (allow.length > 0) {
    return allow.includes(countryCode)
  }
  if (block.length > 0) {
    return !block.includes(countryCode)
  }
  // If region info is invalid (i.e. both the allow list and the block list are
  // empty) optimistically assume that our region data is incorrect and allow
  // the user to attempt to connect a wallet. If the region is not allowed, the
  // process should fail on the server.
  return true
}
