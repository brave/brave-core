// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Helper for calling the Cosmos snap via BraveWalletService.InvokeSnap.
// This follows the same code path as window.ethereum wallet_invokeSnap
// but is usable from chrome:// pages that have no window.ethereum.

import { getAPIProxy } from '../../../common/async/bridge'
import getWalletPageApiProxy from '../../../page/wallet_page_api_proxy'

export const COSMOS_SNAP_ID = 'npm:@cosmsnap/snap'

export const COSMOS_HUB_CHAIN_INFO = {
  chain_name: 'Cosmos Hub',
  chain_id: 'cosmoshub-4',
  slip44: 118,
  bech32_prefix: 'cosmos',
  fees: {
    fee_tokens: [
      {
        denom: 'uatom',
        low_gas_price: 0.01,
        average_gas_price: 0.025,
        high_gas_price: 0.04,
      },
    ],
  },
  staking: { staking_tokens: [{ denom: 'uatom' }] },
  apis: {
    rpc: [{ address: 'https://cosmos-rpc.publicnode.com:443' }],
    rest: [{ address: 'https://cosmos-rest.publicnode.com' }],
  },
}

export interface SnapResponse<T = unknown> {
  data: T
  success: boolean
  statusCode: number
}

/**
 * Calls a snap RPC method via BraveWalletService.InvokeSnap (C++).
 * The C++ side routes through SnapController → SnapBridge → iframe.
 */
export async function callSnap<T = unknown>(
  method: string,
  params?: Record<string, unknown>,
): Promise<SnapResponse<T>> {
  const { braveWalletService } = getAPIProxy()
  // Cast to any — generated types update after next C++ build.
  const invokeSnap = (braveWalletService as any).invokeSnap.bind(braveWalletService)
  const { resultJson, error } = await invokeSnap(
    COSMOS_SNAP_ID,
    method,
    JSON.stringify(params ?? {}),
  )
  if (error) {
    throw new Error(error)
  }
  return JSON.parse(resultJson ?? 'null') as SnapResponse<T>
}

/**
 * Fetch ATOM balance by proxying through the snap executor iframe.
 * chrome://wallet CSP blocks direct external fetches, but the snap executor
 * iframe has connect-src * and relays HTTP GET requests back to us.
 */
export async function fetchAtomBalance(address: string): Promise<string> {
  if (!address) {
    return 'N/A'
  }
  try {
    const { snapBridge } = getWalletPageApiProxy()
    const url = `https://cosmos-rest.publicnode.com/cosmos/bank/v1beta1/balances/${address}`
    const text = await snapBridge.proxyFetch(COSMOS_SNAP_ID, url)
    const json = JSON.parse(text)
    const uatom = (json.balances as Array<{ denom: string; amount: string }>)
      ?.find((b) => b.denom === 'uatom')
    if (!uatom) {
      return '0.000000'
    }
    return (Number(uatom.amount) / 1_000_000).toFixed(6)
  } catch {
    return 'N/A'
  }
}
