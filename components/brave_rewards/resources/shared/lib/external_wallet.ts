/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export type ExternalWalletProvider =
  'uphold' |
  'bitflyer' |
  'gemini'

export type ExternalWalletStatus =
  'pending' |
  'verified' |
  'disconnected'

export interface ExternalWallet {
  provider: ExternalWalletProvider
  status: ExternalWalletStatus
  username: string
  links: {
    account?: string
    addFunds?: string
    completeVerification?: string
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
  function mapStatus (status: number): ExternalWalletStatus | null {
    switch (status) {
      case 1: // CONNECTED
      case 2: // VERIFIED
        return 'verified'
      case 3: // DISCONNECTED_NOT_VERIFIED
      case 4: // DISCONNECTED_VERIFIED
        return 'disconnected'
      case 5: // PENDING
        return 'pending'
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
      addFunds: String(data.addUrl || ''),
      completeVerification: String(data.verifyUrl || '')
    }
  }
}
