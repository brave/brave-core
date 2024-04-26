/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/crx_file/crx_creator.h"

namespace crx_file {

class CrxFileHeader;

std::string GetCrxId_BraveImpl(const std::string& key, CrxFileHeader* header);

}  // namespace crx_file

#include "src/components/crx_file/crx_creator.cc"

namespace crx_file {

// Override for GetCrxId() in SignArchiveAndCreateHeader() to generate the
// correct signed data for the second signature.
std::string GetCrxId_BraveImpl(const std::string& key, CrxFileHeader* header) {
  if (header->sha256_with_rsa_size() > 0) {
    const AsymmetricKeyProof& first_proof = header->sha256_with_rsa()[0];
    return GetCrxId(first_proof.public_key());
  }
  return GetCrxId(key);
}

CreatorResult CreateWithMultipleKeys(const base::FilePath& output_path,
                                     const base::FilePath& zip_path,
                                     std::vector<crypto::RSAPrivateKey*> keys) {
  CrxFileHeader header;
  base::File file(zip_path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  for (auto key : keys) {
    CHECK(key);
    file.Seek(base::File::Whence::FROM_BEGIN, 0);
    const CreatorResult signing_result =
        SignArchiveAndCreateHeader(output_path, &file, key, &header);
    if (signing_result != CreatorResult::OK) {
      return signing_result;
    }
  }

  return WriteCRX(header, output_path, &file);
}

}  // namespace crx_file
