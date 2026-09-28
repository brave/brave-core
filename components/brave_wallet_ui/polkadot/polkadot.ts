import {
  TypeRegistry,
  Metadata,
  GenericChainProperties,
} from '@polkadot/types'

async function rpcCall(rpcUrl: string, method: string, params = []) {
  const res = await fetch(rpcUrl, {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify({ id: 1, jsonrpc: '2.0', method, params }),
  })
  const body = await res.json()
  if (body.error) {
    throw new Error(`RPC error: ${JSON.stringify(body.error)}`)
  }
  return body.result
}

const registry = new TypeRegistry()

async function applyChainProperties(rpcUrl: string) {
  const props = await rpcCall(rpcUrl, 'system_properties')

  registry.setChainProperties(
    new GenericChainProperties(registry, {
      ss58Format: props.ss58Format,
      tokenDecimals: props.tokenDecimals,
      tokenSymbol: props.tokenSymbol,
    }),
  )
}

async function applyMetadata(hex: `0x${string}`, rpcUrl: string) {
  const metadata = new Metadata(registry, hex)
  registry.setMetadata(metadata)
  await applyChainProperties(rpcUrl)
}

const rpcUrl = 'https://polkadot-asset-hub-rpc.polkadot.io';

const metadataHex = await rpcCall(rpcUrl, 'state_getMetadata')
await applyMetadata(metadataHex, rpcUrl)

const methodHex = '0x0a030032fffa4729e6d1447a62c39b997ad75ddbaf80455ea44719b58ee4c03f6a14024913'

const call = registry.createType('Call', methodHex)
console.log(JSON.stringify(call.toHuman(), null, 2))
