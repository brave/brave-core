// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { pkcs7, pki, asn1, util } from 'node-forge';
import SignPdfError from './SignPdfError';
import { removeTrailingNewLine, plainAddPlaceholder, DEFAULT_BYTE_RANGE_PLACEHOLDER } from 'node-signpdf';
import { PDFDocument, PDFPage, StandardFonts, rgb } from 'pdf-lib';
import { SelectionCoords } from './pdf_renderer';

const certString = `-----BEGIN CERTIFICATE-----
MIIF4zCCBMugAwIBAgIGdz2M2uUGMA0GCSqGSIb3DQEBCwUAMHsxCzAJBgNVBAYT
AklOMSgwJgYDVQQKEx9GdXR1cmlRIFN5c3RlbXMgUHJpdmF0ZSBMaW1pdGVkMQ8w
DQYDVQQLEwZTdWItQ0ExMTAvBgNVBAMTKFNpZ25YIHN1Yi1DQSBmb3IgQ2xhc3Mg
MyBJbmRpdmlkdWFsIDIwMjIwHhcNMjQwMTE1MDkzMTAyWhcNMjYwMTE1MDkzMTAy
WjCCAS8xCzAJBgNVBAYTAklOMREwDwYDVQQKEwhQZXJzb25hbDENMAsGA1UEDBME
MzE4OTEpMCcGA1UEQRMgdVpvRkcwZXEzRktZQTlWSUQ4R1FvNHBQNTdsY0tGaHQx
STBHBgNVBBQTQDdiODc5M2Q5MzU4YzZlMGRlZWVjMzZlZjIxNDQxMjQ2M2Y1MTYx
MzVhOWRjMDcyYjJmNmFlYzEwYTJjMjkxMTMxDzANBgNVBBETBjIwMTAwMjEWMBQG
A1UECBMNVXR0YXIgUHJhZGVzaDFJMEcGA1UEBRNANTUzNDExODQ1Y2JhMTU4ZTdl
NGMxMDYxNTNhMzFiZDY4OWIzOGM5MTEyNDUwN2UwNDRmM2M5ZDA5NDAzMDFjMjEU
MBIGA1UEAxMLVWRheSBCYW5zYWwwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
AoIBAQC2/UPH6hpzFUEr/D2qVjFMGIHm4YCKICFLKUvnJoAI8RfYlED5upCeNPeI
woqey7maVUahcFqasMZQkDEw1h0oveg4p1xLpew/WfwnJ41kl0YCORmKnLvMmttG
Pp5Qp0RjAJPwhgYqzuKUduYQ+fvKhphDe2ivFs175y4IvoNliJQ+4e/OTM5ebwcR
p/FsBksDkkaNXJsd4cOsKUNziJNa8mb/eb7/elYpLZRgIWJLJnI6Kq5yCNWZLsCe
mSlAns2Ys4swVH1xwMdPCEvZQljdR5wO2fqGR00rzpSgakJfcClH+nhFpqZtokcq
xez7OCLZ3eWmKrZcfIcSllrJuTGxAgMBAAGjggG1MIIBsTATBgNVHSMEDDAKgAhA
Sva0zedCjDARBgNVHQ4ECgQITDVERpz3ZBAwgZUGCCsGAQUFBwEBBIGIMIGFMEgG
CCsGAQUFBzAChjxodHRwczovL3NpZ254Y2EuY29tL3JlcG9zaXRvcnkvYWlhL1Np
Z25YQ2xhc3MzSW5kaXZpZHVhbC5jZXIwOQYIKwYBBQUHMAGGLWh0dHA6Ly9vY3Nw
LnNpZ254Y2EuY29tL1NpZ25YQ2xhc3MzSW5kaXZpZHVhbDBNBgNVHR8ERjBEMEKg
QKA+hjxodHRwczovL3NpZ254Y2EuY29tL3JlcG9zaXRvcnkvY3JsL1NpZ25YQ2xh
c3MzSW5kaXZpZHVhbC5jcmwwHQYDVR0gBBYwFDAIBgZggmRkAgMwCAYGYIJkZAIC
MAwGA1UdEwEB/wQCMAAwQAYDVR0lBDkwNwYIKwYBBQUHAwIGCCsGAQUFBwMEBgkq
hkiG9y8BAQUGCisGAQQBgjcKAwwGCisGAQQBgjcUAgIwIQYDVR0RBBowGIEWdWRh
eWJhbnNhbDE5QGdtYWlsLmNvbTAOBgNVHQ8BAf8EBAMCBsAwDQYJKoZIhvcNAQEL
BQADggEBAHoCxq9RS5R2GeWhtrgXzhNNuv+HG+oR5auroxC53WHEzPr8LIEsusPi
a47/FM+wvU4tvcLm/6EiDulrulgoG8G1LvW0eWwaMCtI1/XWY9HrVTZXjf1TaQoN
fery9qzcNNXHnNxb0NN6NanZt7lYaRBXZBqxxZ6HH8l2SHdLwJb4i/kHFBbdYZpi
bJgDGTXQnDvFlj8hSr289RK+FiPiDj0DGFkvBNoK65wnpTc/sVSI68XmhjQQ32rZ
Bjc1vCF1dwfxqolhxrYiRfiyRjgGIC52CuI/J6uYuqwk5Cm/NTQW+QNIB4YM9Had
llm7I1xIHHqkwC0uTS8UgfyOzuKN3j0=
-----END CERTIFICATE-----`;

const addPlaceholder = async (
  pdfBuffer: Buffer,
  certificate: pki.Certificate,
  pageIndex: number,
  selectionCoords: SelectionCoords,
): Promise<Buffer> => {
  let pdfDoc: PDFDocument | null = null;
  let page: PDFPage | null = null;
  const { startX, startY, endX, endY } = selectionCoords;
  try {
    pdfDoc = await PDFDocument.load(pdfBuffer);
    page = pdfDoc.getPage(pageIndex);
    const width = endX - startX;
    const height = endY - startY;

    // Draw border
    page.drawRectangle({
      x: startX,
      y: page.getHeight() - endY,
      width: width,
      height: height,
      borderColor: rgb(0, 0, 0),
      borderWidth: 1,
    });

    const imageHex = "89504e470d0a1a0a0000000d4948445200000092000000920806000000ae7b938e000000097048597300000b1300000b1301009a9c18000000017352474200aece1ce90000000467414d410000b18f0bfc610500000215494441547801eddd316e14411040d1b20d487644c4fd8f681210162b312307e4deef96b6f73da94f305f5351753fcccc8fe33c0d7cdce57144c4f59e1e07024222212412422221241242222124124222212412422221241242222124124222212412422221241242222124124222212412422221241242222124125f66adcb71fe0c2b3c1fe76116591dd2dfe3fc1c5638bfedb759c468232124124222212412422221241242222124124222212412422221241242222124124222212412422221241242222124124222212412abd7913edbf91ae6b2159c2bbdcdfb9edf16760be98ce8fbdc86d7e3fc9e4d186d2476fb23f1dfaf59b81e2fa47d2d1d9b461b09219110120921911012092191101209219110120921911012092191101209219110120921911012092191101209219110120921911012092191101209219110120921911012092191101209219110120921911012097748eeeb79deef1d5f4248fb7a998597d71b6d2476fb239dcf32bcce6d789b8dec16d2f9b6c736cf32dc12a38d8490480889849048088984904808898490480889849048088984904808898490480889849048088984904808898490480889849048dcdba6edb9c6fc32f7e1eb2c746f219dbbf0cbd698ef89d146424824844442482484444248248444424824844442482484444248248444424824844442482484444248248444424824844442482484444248248444e20ce932709dcb3ff70414419c1505c80000000049454e44ae426082";
    const imageBytes = Buffer.from(imageHex, 'hex');
    const image = await pdfDoc.embedPng(imageBytes);
    const scaleFactor =
      Math.min(width / image.width, height / image.height) * 0.8;
    page.drawImage(image, {
      x: startX + (width - image.width * scaleFactor) / 2,
      y:
        page.getHeight() - startY - (height + image.height * scaleFactor) / 2,
      width: image.width * scaleFactor,
      height: image.height * scaleFactor,
      opacity: 0.8,
    });
    const font = await pdfDoc.embedFont(StandardFonts.HelveticaBold);
    const regularFont = await pdfDoc.embedFont(StandardFonts.Helvetica);

    const now = new Date();
    const timestamp = `${now.toLocaleDateString()}, ${now.toLocaleTimeString()}`;

    // Extract details from certificate
    const commonName = certificate.subject.getField('CN')?.value || 'Unknown';
    const organization = certificate.subject.getField('O')?.value || 'Unknown';
    const email =
      certificate.subject.getField('E')?.value ||
      `${commonName
        .toLowerCase()
        .replace(' ', '')}@${organization.toLowerCase()}.com`;
    const encKey = certificate.serialNumber;

    // Draw text
    const drawText = (text: string, x: number, y: number, size: number, isRegular = false) => {
      if (page) {
        page.drawText(text, {
          x: startX + x,
          y: page.getHeight() - startY - y,
          size: size,
          font: isRegular ? regularFont : font,
          color: rgb(0, 0, 0),
        });
      } else {
        console.log('page value is initialised as null');
      }
    };

    drawText(commonName, 10, 25, 14);
    drawText(`${organization}`, 10, 45, 10, true);
    drawText(email, 10, 60, 10, true);
    drawText(timestamp, 10, 75, 10, true);
    drawText(`Enc. Key: ${encKey}`, 10, 90, 10, true);

    const modifiedPdfBytes = await pdfDoc.save({
      addDefaultPage: false,
      useObjectStreams: false,
    });

    return Buffer.from(modifiedPdfBytes);
  } catch (error) {
    console.error('Error adding placeholder:', error);
    throw error;
  }
};

const getSignablePdfBuffer = (pdfBuffer: Buffer) => {
  pdfBuffer = plainAddPlaceholder({ pdfBuffer, reason: 'I am the author' });
  let pdf = removeTrailingNewLine(pdfBuffer);

  let byteRangePlaceholderStr = DEFAULT_BYTE_RANGE_PLACEHOLDER;
  const byteRangePlaceholder = [
    0,
    `/${byteRangePlaceholderStr}`,
    `/${byteRangePlaceholderStr}`,
    `/${byteRangePlaceholderStr}`
  ];
  const byteRangeString = `/ByteRange [${byteRangePlaceholder.join(' ')}]`;
  const byteRangePos = pdf.indexOf(byteRangeString);
  if (byteRangePos === -1) {
    throw new SignPdfError(
      `Could not find ByteRange placeholder: ${byteRangeString}`,
      SignPdfError.TYPE_PARSE
    );
  }

  const byteRangeEnd = byteRangePos + byteRangeString.length;
  const contentsTagPos = pdf.indexOf('/Contents ', byteRangeEnd);
  const placeholderPos = pdf.indexOf('<', contentsTagPos);
  const placeholderEnd = pdf.indexOf('>', placeholderPos);
  const placeholderLengthWithBrackets = placeholderEnd + 1 - placeholderPos;
  const placeholderLength = placeholderLengthWithBrackets - 2;
  const byteRange = [0, 0, 0, 0];
  byteRange[1] = placeholderPos;
  byteRange[2] = byteRange[1] + placeholderLengthWithBrackets;
  byteRange[3] = pdf.length - byteRange[2];
  let actualByteRange = `/ByteRange [${byteRange.join(' ')}]`;
  actualByteRange += ' '.repeat(byteRangeString.length - actualByteRange.length);

  pdf = Buffer.concat([
    pdf.slice(0, byteRangePos),
    Buffer.from(actualByteRange),
    pdf.slice(byteRangeEnd)
  ]);

  pdf = Buffer.concat([
    pdf.slice(0, byteRange[1]),
    pdf.slice(byteRange[2], byteRange[2] + byteRange[3])
  ]);

  return { pdf, placeholderLength, byteRange };
};

const embedP7inPdf = (pdf: Buffer, p7: pkcs7.PkcsSignedData, byteRange: number[], placeholderLength: number) => {
  const raw = asn1.toDer(p7.toAsn1()).getBytes();
  if (raw.length * 2 > placeholderLength) {
    throw new SignPdfError(
      `Signature exceeds placeholder length: ${raw.length * 2} > ${placeholderLength}`,
      SignPdfError.TYPE_INPUT
    );
  }

  let signature = Buffer.from(raw, 'binary').toString('hex');
  let hexSignature = signature;

  signature += Buffer.from(
    String.fromCharCode(0).repeat(placeholderLength / 2 - raw.length)
  ).toString('hex');

  let signedPdf = Buffer.concat([
    pdf.slice(0, byteRange[1]),
    Buffer.from(`<${signature}>`),
    pdf.slice(byteRange[1])
  ]);

  return { signedPdf, hexSignature };
};

export const signPdf = async (
  pdfBuff: Buffer,
  currentPageIndex: number,
  selectionCoords: SelectionCoords,
  hsmPath: string,
  pin: string
): Promise<Buffer> => {
  // Parse the certificate
  const certificate = pki.certificateFromPem(certString);

  // Add placeholder to the PDF
  const pdfWithPlaceholder = await addPlaceholder(
    pdfBuff,
    certificate,
    currentPageIndex,
    selectionCoords
  );

  // Get the signable PDF buffer
  const { pdf, placeholderLength, byteRange } = getSignablePdfBuffer(pdfWithPlaceholder);

  // Create the signer object
  const signer: any = {
    sign: (md: any) => {
      const prefix = Buffer.from([
        0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03,
        0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20
      ]);
      let buf = Buffer.concat([prefix, Buffer.from(md.digest().toHex(), 'hex')]);
      const toSign = buf.toString('hex');

      let signHex: string = '';
      (chrome as any).pkcs11.getSignature(
        hsmPath,
        pin,
        toSign,
        (sig: string) => (signHex = sig)
      );

      let sigBuffer = Buffer.from(signHex, 'hex');
      return sigBuffer.toString('binary');
    }
  };

  // Create PKCS7 signed data
  const p7 = pkcs7.createSignedData();
  p7.content = util.createBuffer(pdf.toString('binary'));
  p7.addCertificate(certificate);
  p7.addSigner({
    key: signer,
    certificate,
    digestAlgorithm: pki.oids.sha256,
    authenticatedAttributes: [
      {
        type: pki.oids.contentType,
        value: pki.oids.data
      },
      {
        type: pki.oids.messageDigest
      },
      {
        type: pki.oids.signingTime,
        value: new Date().toString()
      }
    ]
  });

  p7.sign({ detached: true });

  // Embed P7 in PDF
  const { signedPdf } = embedP7inPdf(pdf, p7, byteRange, placeholderLength);

  return signedPdf;
};
