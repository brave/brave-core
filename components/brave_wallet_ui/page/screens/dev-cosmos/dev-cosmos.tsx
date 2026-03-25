// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import {
  callSnap,
  fetchAtomBalance,
  COSMOS_HUB_CHAIN_INFO,
  COSMOS_SNAP_ID,
  type SnapResponse,
} from './cosmos_snap'
import getWalletPageApiProxy from '../../../page/wallet_page_api_proxy'

// ---------------------------------------------------------------------------
// Styled components
// ---------------------------------------------------------------------------

const Wrapper = styled.div`
  padding: 24px;
  max-width: 720px;
  font-family: monospace;
  font-size: 13px;
`

const PageTitle = styled.h1`
  font-size: 20px;
  font-weight: 600;
  margin-bottom: 4px;
`

const Subtitle = styled.p`
  color: #666;
  margin: 0 0 24px;
  font-family: sans-serif;
  font-size: 13px;
`

const Section = styled.div`
  border: 1px solid #ddd;
  border-radius: 8px;
  padding: 16px;
  margin-bottom: 16px;
`

const SectionTitle = styled.h2`
  font-size: 14px;
  font-weight: 600;
  margin: 0 0 12px;
  font-family: sans-serif;
`

const Row = styled.div`
  display: flex;
  align-items: center;
  gap: 10px;
  margin-bottom: 8px;
  flex-wrap: wrap;
`

const Label = styled.span`
  color: #555;
  min-width: 100px;
`

const Value = styled.span`
  font-weight: 500;
  word-break: break-all;
`

const Badge = styled.span<{ ok?: boolean }>`
  display: inline-block;
  padding: 2px 8px;
  border-radius: 12px;
  font-size: 12px;
  background: ${({ ok }) => (ok ? '#d4edda' : '#f8d7da')};
  color: ${({ ok }) => (ok ? '#155724' : '#721c24')};
`

const Btn = styled.button<{ variant?: 'primary' | 'danger' }>`
  padding: 6px 14px;
  border-radius: 6px;
  border: 1px solid
    ${({ variant }) => (variant === 'danger' ? '#dc3545' : '#0c5460')};
  background: ${({ variant }) =>
    variant === 'danger' ? '#dc3545' : '#17a2b8'};
  color: white;
  cursor: pointer;
  font-size: 12px;
  font-family: sans-serif;
  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
`

const Input = styled.input`
  padding: 6px 10px;
  border: 1px solid #ccc;
  border-radius: 6px;
  font-family: monospace;
  font-size: 12px;
  flex: 1;
  min-width: 200px;
`

const LogPre = styled.pre`
  background: #1e1e1e;
  color: #d4d4d4;
  padding: 12px;
  border-radius: 6px;
  max-height: 320px;
  overflow-y: auto;
  font-size: 11px;
  margin: 0;
  white-space: pre-wrap;
  word-break: break-all;
`

const ErrorMsg = styled.div`
  color: #721c24;
  background: #f8d7da;
  border: 1px solid #f5c6cb;
  border-radius: 6px;
  padding: 8px 12px;
  margin-top: 8px;
  font-size: 12px;
  font-family: sans-serif;
`

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

interface LogEntry {
  time: string
  method: string
  params?: unknown
  result?: unknown
  error?: string
  durationMs: number
}

// ---------------------------------------------------------------------------
// Component
// ---------------------------------------------------------------------------

export function DevCosmos() {
  const [initialized, setInitialized] = React.useState<boolean | null>(null)
  const [address, setAddress] = React.useState<string | null>(null)
  const [balance, setBalance] = React.useState<string | null>(null)
  const [chains, setChains] = React.useState<unknown[] | null>(null)
  const [recipient, setRecipient] = React.useState('')
  const [amount, setAmount] = React.useState('')
  const [log, setLog] = React.useState<LogEntry[]>([])
  const [loading, setLoading] = React.useState<Record<string, boolean>>({})
  const [lastError, setLastError] = React.useState<string | null>(null)

  const appendLog = React.useCallback(
    (entry: LogEntry) => setLog((prev) => [entry, ...prev]),
    [],
  )

  const invoke = React.useCallback(
    async (
      method: string,
      params?: Record<string, unknown>,
    ): Promise<SnapResponse | null> => {
      setLoading((prev) => ({ ...prev, [method]: true }))
      setLastError(null)
      const t0 = performance.now()
      try {
        const result = await callSnap(method, params)
        appendLog({
          time: new Date().toLocaleTimeString(),
          method,
          params,
          result,
          durationMs: Math.round(performance.now() - t0),
        })
        return result
      } catch (err) {
        const msg = err instanceof Error ? err.message : String(err)
        appendLog({
          time: new Date().toLocaleTimeString(),
          method,
          params,
          error: msg,
          durationMs: Math.round(performance.now() - t0),
        })
        setLastError(`${method}: ${msg}`)
        return null
      } finally {
        setLoading((prev) => ({ ...prev, [method]: false }))
      }
    },
    [appendLog],
  )

  // Check initialization status on mount
  React.useEffect(() => {
    invoke('initialized').then((res) => {
      if (res?.data && typeof (res.data as any).initialized === 'boolean') {
        setInitialized((res.data as any).initialized)
      }
    })
  }, [invoke])

  const handleInitialize = async () => {
    const res = await invoke('initialize')
    if (res?.success) {
      setInitialized(true)
    }
  }

  // Unload the snap iframe — clears all in-memory state (snapStateStore).
  // The next invoke() creates a fresh iframe and the snap starts from scratch.
  const handleReset = () => {
    getWalletPageApiProxy().snapBridge.unloadSnap(COSMOS_SNAP_ID)
    setInitialized(false)
    setChains(null)
    setAddress(null)
    setBalance(null)
    appendLog({
      time: new Date().toLocaleTimeString(),
      method: 'reset',
      result: 'snap iframe unloaded — state cleared, re-initialize to continue',
      durationMs: 0,
    })
  }

  const handleGetChains = async () => {
    const res = await invoke('getChains')
    if (res?.data && (res.data as any).chains) {
      setChains((res.data as any).chains)
    }
  }

  const handleAddCosmosHub = async () => {
    const addRes = await invoke('addChain', {
      chain_info: JSON.stringify(COSMOS_HUB_CHAIN_INFO),
    })
    if (!addRes?.success) {
      // Chain already exists (added by initialize) and its apis.rpc[0] points
      // to a non-CORS endpoint. Delete and re-add with our CORS-friendly config.
      await invoke('deleteChain', { chain_id: 'cosmoshub-4' })
      await invoke('addChain', {
        chain_info: JSON.stringify(COSMOS_HUB_CHAIN_INFO),
      })
    }
    handleGetChains()
  }

  const handleGetAddress = async () => {
    const res = await invoke('getChainAddress', { chain_id: 'cosmoshub-4' })
    const rawAddr = res?.data && (res.data as any).address
    if (rawAddr) {
      // getChainAddress may return a string or an AccountData-like object
      const addr = typeof rawAddr === 'string' ? rawAddr : (rawAddr as any).address ?? String(rawAddr)
      setAddress(addr)
      setBalance(null)  // Reset balance when address changes
    }
  }

  const handleRefreshBalance = async () => {
    if (!address) {
      return
    }
    setLoading((prev) => ({ ...prev, balance: true }))
    try {
      const bal = await fetchAtomBalance(address)
      setBalance(bal)
      appendLog({
        time: new Date().toLocaleTimeString(),
        method: 'fetchBalance (REST)',
        params: { address },
        result: { balance: bal },
        durationMs: 0,
      })
    } finally {
      setLoading((prev) => ({ ...prev, balance: false }))
    }
  }

  const handleSend = async () => {
    if (!address || !recipient || !amount) {
      return
    }
    // Always ensure the chain uses the CORS-friendly RPC before transacting.
    // The chain registry (initializeChains) stores endpoints that block
    // cross-origin requests from chrome-untrusted://.
    await invoke('changeChain', {
      chain_id: 'cosmoshub-4',
      rpc: 'https://cosmos-rpc.publicnode.com:443',
    })
    const uatom = Math.floor(Number(amount) * 1_000_000).toString()
    const msgs = [
      {
        typeUrl: '/cosmos.bank.v1beta1.MsgSend',
        value: {
          fromAddress: address,
          toAddress: recipient,
          amount: [{ denom: 'uatom', amount: uatom }],
        },
      },
    ]
    const fees = {
      amount: [{ denom: 'uatom', amount: '5000' }],
      gas: '200000',
    }
    const res = await invoke('transact', {
      msgs: JSON.stringify(msgs),
      chain_id: 'cosmoshub-4',
      fees: JSON.stringify(fees),
    })
    if (res?.success) {
      handleRefreshBalance()
    }
  }

  const isLoading = (key: string) => loading[key] === true

  return (
    <Wrapper>
      <PageTitle>Cosmos Snap Dev Console</PageTitle>
      <Subtitle>
        brave://wallet/dev-cosmos — calls flow through BraveWalletService →
        SnapController → SnapBridge → SES iframe
      </Subtitle>

      {lastError && <ErrorMsg>{lastError}</ErrorMsg>}

      {/* Section A: Initialization */}
      <Section>
        <SectionTitle>Initialization</SectionTitle>
        <Row>
          <Label>Status:</Label>
          {initialized === null ? (
            <Value>checking…</Value>
          ) : (
            <Badge ok={initialized}>
              {initialized ? 'Initialized' : 'Not initialized'}
            </Badge>
          )}
        </Row>
        <Row>
          <Btn
            onClick={handleInitialize}
            disabled={isLoading('initialize') || initialized === true}
          >
            {isLoading('initialize') ? 'Initializing…' : 'Initialize'}
          </Btn>
          <Btn
            variant='danger'
            onClick={handleReset}
            title='Unloads the snap iframe, clearing all in-memory state'
          >
            Reset State
          </Btn>
          <Btn
            onClick={() =>
              invoke('initialized').then((res) => {
                if (
                  res?.data &&
                  typeof (res.data as any).initialized === 'boolean'
                ) {
                  setInitialized((res.data as any).initialized)
                }
              })
            }
            disabled={isLoading('initialized')}
          >
            Re-check
          </Btn>
        </Row>
      </Section>

      {/* Section B: Chain Management */}
      <Section>
        <SectionTitle>Chain Management</SectionTitle>
        <Row>
          <Btn onClick={handleAddCosmosHub} disabled={isLoading('addChain')}>
            {isLoading('addChain') ? 'Adding…' : 'Add Cosmos Hub'}
          </Btn>
          <Btn onClick={handleGetChains} disabled={isLoading('getChains')}>
            {isLoading('getChains') ? 'Loading…' : 'Get Chains'}
          </Btn>
        </Row>
        {chains !== null && (
          <Row>
            <Label>Chains:</Label>
            <Value>{(chains as any[]).length} chain(s) loaded</Value>
          </Row>
        )}
      </Section>

      {/* Section C: Address */}
      <Section>
        <SectionTitle>Address</SectionTitle>
        <Row>
          <Btn
            onClick={handleGetAddress}
            disabled={isLoading('getChainAddress')}
          >
            {isLoading('getChainAddress') ? 'Deriving…' : 'Get Address'}
          </Btn>
        </Row>
        {address && (
          <Row>
            <Label>cosmos1…:</Label>
            <Value>{address}</Value>
            <Btn
              onClick={() => navigator.clipboard.writeText(address)}
              style={{ padding: '2px 8px', fontSize: '11px' }}
            >
              Copy
            </Btn>
          </Row>
        )}
      </Section>

      {/* Section D: Balance */}
      <Section>
        <SectionTitle>Balance</SectionTitle>
        <Row>
          <Btn
            onClick={handleRefreshBalance}
            disabled={!address || isLoading('balance')}
          >
            {isLoading('balance') ? 'Loading…' : 'Refresh Balance'}
          </Btn>
        </Row>
        {balance !== null && (
          <Row>
            <Label>ATOM:</Label>
            <Value>{balance} ATOM</Value>
          </Row>
        )}
      </Section>

      {/* Section E: Send */}
      <Section>
        <SectionTitle>Send ATOM</SectionTitle>
        <Row>
          <Label>To:</Label>
          <Input
            placeholder='cosmos1recipient…'
            value={recipient}
            onChange={(e) => setRecipient(e.target.value)}
          />
        </Row>
        <Row>
          <Label>Amount (ATOM):</Label>
          <Input
            placeholder='0.001'
            type='number'
            min='0'
            step='0.000001'
            value={amount}
            onChange={(e) => setAmount(e.target.value)}
          />
        </Row>
        <Row>
          <Btn
            onClick={handleSend}
            disabled={!address || !recipient || !amount || isLoading('transact')}
          >
            {isLoading('transact') ? 'Sending…' : 'Send'}
          </Btn>
        </Row>
      </Section>

      {/* Section F: Debug Log */}
      <Section>
        <SectionTitle
          style={{ display: 'flex', justifyContent: 'space-between' }}
        >
          <span>Debug Log ({log.length})</span>
          <Btn
            variant='danger'
            onClick={() => setLog([])}
            style={{ padding: '2px 8px', fontSize: '11px' }}
          >
            Clear
          </Btn>
        </SectionTitle>
        <LogPre>
          {log.length === 0
            ? '// snap calls will appear here'
            : log
                .map(
                  (e) =>
                    `[${e.time}] ${e.method} (${e.durationMs}ms)\n` +
                    (e.params
                      ? `  params: ${JSON.stringify(e.params, null, 2)
                          .split('\n')
                          .join('\n  ')}\n`
                      : '') +
                    (e.result
                      ? `  result: ${JSON.stringify(e.result, null, 2)
                          .split('\n')
                          .join('\n  ')}`
                      : '') +
                    (e.error ? `  error: ${e.error}` : ''),
                )
                .join('\n\n')}
        </LogPre>
      </Section>
    </Wrapper>
  )
}

export default DevCosmos
