// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { asn1, pkcs7, pki } from 'node-forge'
import { createVerify, createHash } from 'crypto'

export const verifyPDF = (pdf: Buffer) => {
  const extractedData = getSignature(pdf)

  const p7Asn1 = asn1.fromDer(extractedData.signature)

  const message = pkcs7.messageFromAsn1(
    p7Asn1
  ) as pkcs7.Captured<pkcs7.PkcsSignedData>

  const {
    signature: sig,
    digestAlgorithm,
    authenticatedAttributes: attrs
  } = message.rawCapture

  const set = asn1.create(asn1.Class.UNIVERSAL, asn1.Type.SET, true, attrs)

  const hashAlgorithmOid = asn1.derToOid(digestAlgorithm)
  const hashAlgorithm = pki.oids[hashAlgorithmOid].toUpperCase()

  const buf = Buffer.from(asn1.toDer(set).data, 'binary')
  const verifier = createVerify(`RSA-${hashAlgorithm}`)
  verifier.update(buf)

  const cert = pki.certificateToPem(message.certificates[0])

  const validAuthenticatedAttributes = verifier.verify(cert, sig, 'binary')

  if (!validAuthenticatedAttributes)
    throw new Error('Wrong authenticated attributes')

  const pdfHash = createHash(hashAlgorithm)

  const data = extractedData.signedData
  pdfHash.update(data)

  const oids = pki.oids
  const fullAttrDigest = attrs.find(
    (attr: any) => asn1.derToOid(attr.value[0].value) === oids.messageDigest
  )
  const attrDigest = fullAttrDigest.value[1].value[0].value

  const dataDigest = pdfHash.digest()

  const validContentDigest = dataDigest.toString('binary') === attrDigest

  if (validContentDigest) {
    return true;
  } else {
    throw new Error('Wrong content digest')
  }
}

const getSignature = (pdf: Buffer) => {
  let byteRangePos = pdf.lastIndexOf('/ByteRange[')
  if (byteRangePos === -1) byteRangePos = pdf.lastIndexOf('/ByteRange [')

  const byteRangeEnd = pdf.indexOf(']', byteRangePos)
  const byteRange = pdf.slice(byteRangePos, byteRangeEnd + 1).toString()
  const byteRangeNumbers = /(\d+) +(\d+) +(\d+) +(\d+)/.exec(byteRange)
  const byteRangeArr = byteRangeNumbers?.[0].split(' ')

  if (!byteRangeArr) throw new Error('Byte range is not found')

  const signedData = Buffer.concat([
    pdf.slice(parseInt(byteRangeArr[0]), parseInt(byteRangeArr[1])),
    pdf.slice(
      parseInt(byteRangeArr[2]),
      parseInt(byteRangeArr[2]) + parseInt(byteRangeArr[3])
    )
  ])

  let signatureHex = pdf
    .slice(
      parseInt(byteRangeArr[0]) + (parseInt(byteRangeArr[1]) + 1),
      parseInt(byteRangeArr[2]) - 1
    )
    .toString('binary')
  signatureHex = signatureHex.replace(/(?:00)*$/, '')
  const signature = Buffer.from(signatureHex, 'hex').toString('binary')

  return { signature, signedData }
}