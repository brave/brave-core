
import { PolkadotBridgeReceiver, PolkadotBridgeUIHandler, PolkadotBridgeInterface, PolkadotDecodedPayload } from 'gen/brave/components/brave_wallet/common/polkadot_bridge.mojom.m.js'
import { PolkadotChainProperties } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'

import {
  TypeRegistry,
  Metadata,
  GenericChainProperties,
} from '@polkadot/types'
import { allExtensions } from '@polkadot/types/extrinsic/signedExtensions'

type HexString = `0x${string}`;

export interface SignerPayloadJSON {
  address: string;
  assetId?: HexString;
  blockHash: HexString;
  blockNumber: HexString;
  era: HexString;
  genesisHash: HexString;
  metadataHash?: HexString;
  method: string;
  mode?: number;
  nonce: HexString;
  specVersion: HexString;
  tip: HexString;
  transactionVersion: HexString;
  signedExtensions: string[];
  version: number;
  withSignedTransaction?: boolean;
}

const FAKE_SIGNATURE = new Uint8Array(256).fill(1)

function buildMockExtrinsic(registry: TypeRegistry, payload: SignerPayloadJSON) {
  const extrinsic = registry.createType(
    'Extrinsic',
    { method: payload.method },
    { version: payload.version },
  )

  return extrinsic.addSignature(payload.address, FAKE_SIGNATURE, payload)
}

// `toU8a(true)` sets polkadot-js's bare flag, which encodes `method` without its
// compact length prefix.
function buildSignaturePayload(
  registry: TypeRegistry,
  payload: SignerPayloadJSON,
) {
  const extrinsicPayload = registry.createType(
    'ExtrinsicPayload',
    payload,
    { version: payload.version },
  )

  return extrinsicPayload.toU8a(true)
}

// Reads V15 rather than `asLatest`: polkadot-js's V15->V16 upgrade misspells the
// key as `implict`, so V16's `implicit` is always the default lookup id 0.
function applyExtensionTypes(registry: TypeRegistry, metadata: Metadata) {
  const identifiers = []
  const userExtensions: Record<string, any> = {}

  for (const { identifier, type, additionalSigned } of metadata.asV15.extrinsic.signedExtensions) {
    const name = identifier.toString()
    identifiers.push(name)

    if (!allExtensions[name]) {
      // An unknown extension's implicit data has nowhere to come from:
      // SignerPayloadJSON is a closed set of named fields and ExtrinsicPayload
      // decodes by field name, so a non-empty implicit would silently encode as
      // the type's default and the chain would reject the signature as BadProof.
      const implicit = registry.createType(
        registry.createLookupType(additionalSigned),
      )
      if (implicit.encodedLength > 0) {
        throw new Error(
          `signed extension '${name}' requires implicit data of type ` +
          `${implicit.toRawType()}, which cannot be sourced from SignerPayloadJSON`,
        )
      }

      const typeName = registry.createLookupType(type)
      userExtensions[name] = { extrinsic: { [name]: typeName }, payload: {} }
    }
  }

  registry.setSignedExtensions(identifiers, userExtensions)
}

const setupPolkadotBridge = () => {
  const uiHandler = PolkadotBridgeUIHandler.getRemote()
  uiHandler.bindPolkadotBridge(receiver.$.bindNewPipeAndPassRemote())
}


class PolkadotBridge implements PolkadotBridgeInterface {
  async decode(
    metadataBytes: number[],
    chainProperties: PolkadotChainProperties,
    rawPayloadJson: string,
  ): Promise<{ decoded: PolkadotDecodedPayload | null }> {
    try {
      const payload: SignerPayloadJSON = JSON.parse(rawPayloadJson);

      console.log('incoming SignerPayloadJSON')
      console.log(JSON.stringify(payload, null ,2))

      const registry = new TypeRegistry()
      registry.setChainProperties(
        new GenericChainProperties(registry, {
          ss58Format: chainProperties.ss58Format,
          tokenDecimals: chainProperties.tokenDecimals,
          tokenSymbol: chainProperties.tokenSymbol,
        }),
      )

      const metadata = new Metadata(registry, new Uint8Array(metadataBytes))
      registry.setMetadata(metadata)
      applyExtensionTypes(registry, metadata)

      const call = registry.createType('Call', payload.method)
      const extrinsic = buildMockExtrinsic(registry, payload)
      const rawSignaturePayload = buildSignaturePayload(registry, payload)
      const signaturePayload = Array.from(rawSignaturePayload)

      const asHuman = JSON.stringify(call.toHuman())
      const mockSignedExtrinsic = Array.from(extrinsic.toU8a())

      return {
        decoded: {
          asHuman,
          mockSignedExtrinsic,
          signaturePayload,
          error: undefined,
        },
      }
    } catch(e) {
      console.error('polkadot bridge decode failed', e)
      return {
        decoded: {
          asHuman: undefined,
          mockSignedExtrinsic: undefined,
          signaturePayload: undefined,
          error: e instanceof Error
            ? `${e.name}: ${e.message}\n${e.stack ?? ''}`
            : String(e),
        }
      }
    }
  }
}

const bridge = new PolkadotBridge()
const receiver = new PolkadotBridgeReceiver(bridge)

setupPolkadotBridge();
