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
