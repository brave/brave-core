// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import SwiftUI
import BraveUI
import Shared
import BraveShared
import CertificateUtilities
import DesignSystem

private struct CertificateTitleView: View {
  let isRootCertificate: Bool
  let evaluationError: String?

  var body: some View {
    VStack {
      HStack(alignment: .firstTextBaseline, spacing: 4.0) {
        if let evaluationError = evaluationError {
          Image(braveSystemName: "leo.warning.triangle-filled")
            .foregroundColor(Color(braveSystemName: .systemfeedbackErrorIcon))
          Text(evaluationError)
            .foregroundColor(Color(braveSystemName: .systemfeedbackErrorText))
            .lineLimit(nil)
        } else {
          Image(braveSystemName: "leo.verification.outline")
            .foregroundColor(Color(braveSystemName: .systemfeedbackSuccessIcon))
          Text(Strings.CertificateViewer.certificateIsValidTitle)
            .foregroundColor(Color(braveSystemName: .systemfeedbackSuccessText))
            .lineLimit(nil)
        }
      }
      .font(.callout)
      .multilineTextAlignment(.center)
      .fixedSize(horizontal: false, vertical: true)
    }
    .frame(maxWidth: .infinity)
  }
}

private struct CertificateKeyValueView: View, Hashable {
  let title: String
  let value: String?

  var body: some View {
    HStack(spacing: 12.0) {
      Text(title)
        .font(.system(.caption, design: .monospaced))
      Spacer()
      if let value = value, !value.isEmpty {
        Text(value)
          .fixedSize(horizontal: false, vertical: true)
          .font(.system(.caption, design: .monospaced).weight(.medium))
          .multilineTextAlignment(.trailing)
      }
    }
  }
}

private struct CertificateSectionView<ContentView>: View where ContentView: View & Hashable {
  let title: String
  let values: [ContentView]

  var body: some View {
    Section(
      header: Text(title)
        .font(.system(.caption, design: .monospaced))
    ) {

      ForEach(values, id: \.self) {
        $0.listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
  }
}

private struct CertificateView: View {
  let model: BraveCertificateModel
  let evaluationError: String?

  @Environment(\.dismiss) private var dismiss
  
  var body: some View {
    NavigationView {
      List {
        Section(header: Color.clear.frame(height: 0)) {
          CertificateTitleView(
            isRootCertificate: model.isRootCertificate,
            evaluationError: evaluationError
          )
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        content
      }
      .listStyle(InsetGroupedListStyle())
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .toolbar {
        ToolbarItemGroup(placement: .navigationBarTrailing) {
          Button(Strings.done) {
            dismiss()
          }
        }
      }
      .navigationTitle(model.subjectName.commonName)
      .navigationBarTitleDisplayMode(.inline)
    }
    .navigationViewStyle(.stack)
  }

  @ViewBuilder
  private var content: some View {
    // Subject name
    CertificateSectionView(title: Strings.CertificateViewer.subjectNameTitle, values: subjectNameViews())

    // Issuer name
    CertificateSectionView(
      title: Strings.CertificateViewer.issuerNameTitle,
      values: issuerNameViews())

    // Common info
    CertificateSectionView(
      title: Strings.CertificateViewer.commonInfoTitle,
      values: [
        // Serial number
        CertificateKeyValueView(
          title: Strings.CertificateViewer.serialNumberTitle,
          value: formattedSerialNumber),

        // Version
        CertificateKeyValueView(
          title: Strings.CertificateViewer.versionNumberTitle,
          value: "\(model.version)"),

        // Signature Algorithm

        CertificateKeyValueView(
          title: Strings.CertificateViewer.signatureAlgorithmTitle,
          value: "\(signatureAlgorithmDescription) (\(model.signature.absoluteObjectIdentifier.isEmpty ? BraveCertificateUtils.oid_to_absolute_oid(oid: model.signature.objectIdentifier) : model.signature.absoluteObjectIdentifier))"),

        // Signature Algorithm Parameters
        signatureParametersView(),
      ])

    // Validity info
    CertificateSectionView(
      title: Strings.CertificateViewer.validityDatesTitle,
      values: [
        // Not Valid Before
        CertificateKeyValueView(
          title: Strings.CertificateViewer.notValidBeforeTitle,
          value: BraveCertificateUtils.formatDate(model.notValidBefore)),

        // Not Valid After
        CertificateKeyValueView(
          title: Strings.CertificateViewer.notValidAfterTitle,
          value: BraveCertificateUtils.formatDate(model.notValidAfter)),
      ])

    // Public Key Info
    CertificateSectionView(
      title: Strings.CertificateViewer.publicKeyInfoTitle,
      values: publicKeyInfoViews())

    // Signature
    CertificateSectionView(
      title: Strings.CertificateViewer.signatureTitle,
      values: [
        CertificateKeyValueView(
          title: Strings.CertificateViewer.signatureTitle,
          value: formattedSignature())
      ])

    // Fingerprints
    CertificateSectionView(
      title: Strings.CertificateViewer.fingerPrintsTitle,
      values: fingerprintViews())
  }
}

extension CertificateView {
  private var signatureAlgorithmDescription: String {
    // TODO: Export Enum for this.
    if model.signature.algorithm == "ECDSA" {
      return String(format: Strings.CertificateViewer.signatureAlgorithmSignatureDescription, model.signature.algorithm, model.signature.digest)
    }

    return String(format: Strings.CertificateViewer.signatureAlgorithmEncryptionDescription, model.signature.digest, model.signature.algorithm)
  }

  private var formattedSerialNumber: String {
    let serialNumber = model.serialNumber
    if Int64(serialNumber) != nil || UInt64(serialNumber) != nil {
      return "\(serialNumber)"
    }
    return BraveCertificateUtils.formatHex(model.serialNumber)
  }

  private func rdnsSequenceViews(rdns: BraveCertificateRDNSequence) -> [CertificateKeyValueView] {
    // Ordered mapping
    var mapping = [
      KeyValue(key: Strings.CertificateViewer.countryOrRegionTitle, value: rdns.countryOrRegion),
      KeyValue(key: Strings.CertificateViewer.stateOrProvinceTitle, value: rdns.stateOrProvince),
      KeyValue(key: Strings.CertificateViewer.localityTitle, value: rdns.locality),
    ]

    mapping.append(
      contentsOf: rdns.organization.map {
        KeyValue(key: Strings.CertificateViewer.organizationTitle, value: $0)
      })

    mapping.append(
      contentsOf: rdns.organizationalUnit.map {
        KeyValue(key: Strings.CertificateViewer.organizationalUnitTitle, value: $0)
      })

    mapping.append(KeyValue(key: Strings.CertificateViewer.commonNameTitle, value: rdns.commonName))

    mapping.append(KeyValue(key: Strings.CertificateViewer.userIDTitle, value: rdns.userId))

    return mapping.compactMap({
      $0.value.isEmpty
        ? nil
        : CertificateKeyValueView(
          title: $0.key,
          value: $0.value)
    })
  }

  private func subjectNameViews() -> [CertificateKeyValueView] {
    rdnsSequenceViews(rdns: model.subjectName)
  }

  private func issuerNameViews() -> [CertificateKeyValueView] {
    rdnsSequenceViews(rdns: model.issuerName)
  }

  private func signatureParametersView() -> CertificateKeyValueView {
    let signature = model.signature
    let parameters = signature.parameters.isEmpty ? Strings.CertificateViewer.noneTitle : BraveCertificateUtils.formatHex(signature.parameters)
    return CertificateKeyValueView(
      title: Strings.CertificateViewer.parametersTitle,
      value: parameters)
  }

  private func publicKeyInfoViews() -> [CertificateKeyValueView] {
    let publicKeyInfo = model.publicKeyInfo

    var algorithm = publicKeyInfo.algorithm
    if !publicKeyInfo.curveName.isEmpty {
      algorithm += " - \(publicKeyInfo.curveName)"
    }

    if !algorithm.isEmpty {
      algorithm += " \(Strings.CertificateViewer.encryptionTitle) "
      if publicKeyInfo.absoluteObjectIdentifier.isEmpty {
        algorithm += " (\(BraveCertificateUtils.oid_to_absolute_oid(oid: publicKeyInfo.objectIdentifier)))"
      } else {
        algorithm += " (\(publicKeyInfo.absoluteObjectIdentifier))"
      }
    }

    let parameters = publicKeyInfo.parameters.isEmpty ? Strings.CertificateViewer.noneTitle : "\(publicKeyInfo.parameters.count / 2) \(Strings.CertificateViewer.bytesUnitTitle) : \(BraveCertificateUtils.formatHex(publicKeyInfo.parameters))"

    let publicKey = "\(publicKeyInfo.keyBytesSize) \(Strings.CertificateViewer.bytesUnitTitle) : \(BraveCertificateUtils.formatHex(publicKeyInfo.keyHexEncoded))"

    let keySizeInBits = "\(publicKeyInfo.keySizeInBits) \(Strings.CertificateViewer.bitsUnitTitle)"

    var keyUsages = [String]()
    if publicKeyInfo.keyUsage.contains(.ENCRYPT) {
      keyUsages.append(Strings.CertificateViewer.encryptTitle)
    }

    if publicKeyInfo.keyUsage.contains(.VERIFY) {
      keyUsages.append(Strings.CertificateViewer.verifyTitle)
    }

    if publicKeyInfo.keyUsage.contains(.WRAP) {
      keyUsages.append(Strings.CertificateViewer.wrapTitle)
    }

    if publicKeyInfo.keyUsage.contains(.DERIVE) {
      keyUsages.append(Strings.CertificateViewer.deriveTitle)
    }

    if publicKeyInfo.keyUsage.isEmpty || publicKeyInfo.keyUsage == .INVALID || publicKeyInfo.keyUsage.contains(.ANY) {
      keyUsages.append(Strings.CertificateViewer.anyTitle)
    }

    let exponent = publicKeyInfo.type == .RSA && publicKeyInfo.exponent != 0 ? "\(publicKeyInfo.exponent)" : ""

    // Ordered mapping
    let mapping = [
      KeyValue(key: Strings.CertificateViewer.algorithmTitle, value: algorithm),
      KeyValue(key: Strings.CertificateViewer.parametersTitle, value: parameters),
      KeyValue(key: Strings.CertificateViewer.publicKeyTitle, value: publicKey),
      KeyValue(key: Strings.CertificateViewer.exponentTitle, value: exponent),
      KeyValue(key: Strings.CertificateViewer.keySizeTitle, value: keySizeInBits),
      KeyValue(key: Strings.CertificateViewer.keyUsageTitle, value: keyUsages.joined(separator: " ")),
    ]

    return mapping.compactMap({
      $0.value.isEmpty
        ? nil
        : CertificateKeyValueView(
          title: $0.key,
          value: $0.value)
    })
  }

  private func formattedSignature() -> String {
    let signature = model.signature
    return "\(signature.bytesSize) \(Strings.CertificateViewer.bytesUnitTitle) : \(BraveCertificateUtils.formatHex(signature.signatureHexEncoded))"
  }

  private func fingerprintViews() -> [CertificateKeyValueView] {
    let sha256Fingerprint = model.sha256Fingerprint
    let sha1Fingerprint = model.sha1Fingerprint

    return [
      CertificateKeyValueView(title: Strings.CertificateViewer.sha256Title, value: BraveCertificateUtils.formatHex(sha256Fingerprint.fingerprintHexEncoded)),
      CertificateKeyValueView(title: Strings.CertificateViewer.sha1Title, value: BraveCertificateUtils.formatHex(sha1Fingerprint.fingerprintHexEncoded)),
    ]
  }

  private struct KeyValue {
    let key: String
    let value: String
  }
}

#if DEBUG
struct CertificateView_Previews: PreviewProvider {
  static var previews: some View {
    let model = BraveCertificateModel(name: "leaf")!

    CertificateView(model: model, evaluationError: nil)
  }
}
#endif

class CertificateViewController: UIViewController, PopoverContentComponent {

  init(certificate: BraveCertificateModel, evaluationError: String?) {
    super.init(nibName: nil, bundle: nil)

    let rootView = CertificateView(model: certificate,
                                   evaluationError: evaluationError)
    let controller = UIHostingController(rootView: rootView)

    addChild(controller)
    view.addSubview(controller.view)
    controller.didMove(toParent: self)

    controller.view.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
