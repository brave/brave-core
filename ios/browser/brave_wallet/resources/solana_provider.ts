// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  $Array,
  $Object,
} from '//brave/ios/web/js_messaging/resources/safe_builtins.js'
import { sendTokenizedWebKitMessageWithReply } from '//brave/ios/web/js_messaging/resources/utils.js'

// The hidden namespace that holds the injected `solanaWeb3` module. This is a
// global lexical binding (not a property of `window`) declared and assigned by
// the companion `solanaWeb3` script injected just before this one.
declare const _braveSolanaWeb3: { solanaWeb3: any } | undefined

type Completion = (result: any, resolve: (value: any) => void) => void

if (window.isSecureContext) {
  // If the `solanaWeb3` module failed to inject there is nothing to expose.
  if (typeof _braveSolanaWeb3 !== 'undefined' && _braveSolanaWeb3.solanaWeb3) {
    const solanaWeb3 = _braveSolanaWeb3.solanaWeb3

    function post(
      method: string,
      payload: any,
      completion?: Completion,
    ): Promise<any> {
      return new Promise(
        (resolve: (value: any) => void, reject: (reason: any) => void) => {
          sendTokenizedWebKitMessageWithReply('SolanaProviderMessageHandler', {
            method: method,
            args: JSON.stringify(payload, (_key, value) => {
              // JSON.stringify converts Uint8Array to a dictionary, converting
              // to an array yields a list instead.
              return value instanceof Uint8Array ? $Array.from(value) : value
            }),
          }).then(
            (result: any) => {
              if (completion === undefined) {
                resolve(result)
              } else {
                completion(result, resolve)
              }
            },
            (errorJSON: any) => {
              /* remove `Error: ` prefix. errorJSON=`Error: {code: 1, message: "Internal error"}` */
              const errorJSONString = new String(errorJSON)
              const errorJSONStringSliced = errorJSONString.slice(
                errorJSONString.indexOf('{'),
              )
              try {
                reject(JSON.parse(errorJSONStringSliced))
              } catch (e) {
                reject(errorJSON)
              }
            },
          )
        },
      )
    }

    /*
     solanaWeb3.Transaction | solanaWeb3.VersionedTransaction
     ->
     [UInt8]
     */
    function serializedMessageFromTx(transaction: any): number[] {
      // VersionedTransaction (v0) : Transaction (legacy)
      const serializeMessageBuffer = transaction.message
        ? transaction.message.serialize()
        : transaction.serializeMessage()
      return [...serializeMessageBuffer] // Buffer to Array
    }

    /*
     solanaWeb3.Transaction | solanaWeb3.VersionedTransaction
     ->
     [{publicKey: <base58 encoded string>, signature: [UInt8]}]
     */
    function signaturesPubKeyPairsFromTx(transaction: any): any[] {
      if (transaction.message) {
        // VersionedTransaction (v0)
        const signatures = transaction.signatures
        const versionedMessage = transaction.message
        const staticAccountKeys = versionedMessage.staticAccountKeys

        const signaturePubkeyObjects = []
        for (let i = 0; i < signatures.length; i++) {
          const signature = signatures[i]
          const publicKey = staticAccountKeys[i]

          const obj = $Object.create(null)
          obj.publicKey = publicKey.toBase58()
          obj.signature = signature
          signaturePubkeyObjects[i] = obj
        }

        return signaturePubkeyObjects
      } else {
        // Transaction (legacy)
        const convertSignaturePubkeyTuple = function (
          signaturePubkeyTuple: any,
        ) {
          const obj = $Object.create(null)
          obj.publicKey = signaturePubkeyTuple.publicKey.toBase58()
          const signatureBuffer = signaturePubkeyTuple.signature
          if (signatureBuffer) {
            obj.signature = [...signatureBuffer] // Buffer to Array
          } else {
            obj.signature = []
          }
          return obj
        }
        const txSignatures = $Array.of(...transaction.signatures)
        return txSignatures.map(convertSignaturePubkeyTuple)
      }
    }

    /*
     solanaWeb3.Transaction | solanaWeb3.VersionedTransaction
     ->
     {
      serializedMessage: [UInt8],
      signatures: [{publicKey: <base58 encoded string>, signature: [UInt8]}]
     }
    */
    function convertTransaction(transaction: any): any {
      const serializedMessage = serializedMessageFromTx(transaction)
      const signatures = signaturesPubKeyPairsFromTx(transaction)
      const object = $Object.create(null)
      object.serializedMessage = serializedMessage
      object.signatures = signatures
      return object
    }

    function createTransaction(serializedTxDict: any): any {
      const version = serializedTxDict['version']
      const serializedTx = serializedTxDict['serializedTx']

      if (version === 0) {
        // Transaction (legacy)
        return solanaWeb3.Transaction.from(new Uint8Array(serializedTx))
      }
      // VersionedTransaction (v0)
      return solanaWeb3.VersionedTransaction.deserialize(
        new Uint8Array(serializedTx),
      )
    }

    const provider = {
      value: {
        /* Properties */
        isPhantom: true,
        isBraveWallet: true,
        isConnected: false,
        publicKey: null,
        /* Methods */
        connect: function (payload: any) {
          /* -> {publicKey: solanaWeb3.PublicKey} */
          function completion(publicKey: any, resolve: (value: any) => void) {
            /* convert `<base58 encoded string>` -> `{publicKey: solanaWeb3.PublicKey}` */
            const result = $Object.create(null)
            result.publicKey = new solanaWeb3.PublicKey(publicKey)
            resolve(result)
          }
          return post('connect', payload, completion)
        },
        disconnect: function (payload: any) {
          /* -> Promise<{}> */
          return post('disconnect', payload)
        },
        signAndSendTransaction: function (...payload: any[]) {
          /* -> Promise<{publicKey: <base58 encoded string>, signature: <base58 encoded string>}> */
          const object = convertTransaction(payload[0])
          object.sendOptions = payload[1]
          return post('signAndSendTransaction', object)
        },
        signMessage: function (...payload: any[]) {
          /* -> Promise<{publicKey: solanaWeb3.PublicKey, signature: [UInt8]}> */
          function completion(result: any, resolve: (value: any) => void) {
            /* convert `{publicKey: <base58 encoded string>, signature: [UInt8]}}` ->
             `{publicKey: solanaWeb3.PublicKey, signature: [UInt8]}` */
            const obj = $Object.create(null)
            obj.publicKey = new solanaWeb3.PublicKey(result.publicKey)
            obj.signature = new Uint8Array(result.signature)
            resolve(obj)
          }
          return post('signMessage', payload, completion)
        },
        request: function (args: any) {
          /* -> Promise<unknown> */
          if (args['method'] === 'connect') {
            function completion(publicKey: any, resolve: (value: any) => void) {
              /* convert `<base58 encoded string>` -> `{publicKey: solanaWeb3.PublicKey}` */
              const result = $Object.create(null)
              result.publicKey = new solanaWeb3.PublicKey(publicKey)
              resolve(result)
            }
            return post('request', args, completion)
          }
          return post('request', args)
        },
        /* Deprecated */
        signTransaction: function (transaction: any) {
          /* -> Promise<solanaWeb3.Transaction> */
          const object = convertTransaction(transaction)

          function completion(
            serializedTx: any,
            resolve: (value: any) => void,
          ) {
            /* Convert `[UInt8]` -> `solanaWeb3.Transaction` */
            const result = createTransaction(serializedTx)
            resolve(result)
          }
          return post('signTransaction', object, completion)
        },
        /* Deprecated */
        signAllTransactions: function (transactions: any) {
          /* -> Promise<[solanaWeb3.Transaction]> */
          const objects = $Array.of(...transactions).map(convertTransaction)

          function completion(
            serializedTxs: any,
            resolve: (value: any) => void,
          ) {
            /* Convert `[[UInt8]]` -> `[solanaWeb3.Transaction]` */
            const result = $Array.of(...serializedTxs).map(createTransaction)
            resolve(result)
          }
          return post('signAllTransactions', objects, completion)
        },
      },
    }
    $Object.defineProperty(window, 'solana', provider)
    $Object.defineProperty(window, 'braveSolana', provider)
  }
}
