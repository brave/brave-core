/*
* X.509 Public Key
* (C) 1999-2010,2023 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_X509_PUBLIC_KEY_H_
#define BOTAN_X509_PUBLIC_KEY_H_

#include <botan/data_src.h>
#include <botan/pk_keys.h>
#include <string>
#include <vector>

namespace Botan::X509 {

/**
* BER encode a key
* @param key the public key to encode
* @return BER encoding of this key
*/
inline std::vector<uint8_t> BER_encode(const Public_Key& key) {
   return key.subject_public_key();
}

/**
* PEM encode a public key into a string.
* @param key the key to encode
* @return PEM encoded key
*/
BOTAN_PUBLIC_API(2, 0) std::string PEM_encode(const Public_Key& key);

/**
* Create a public key from a data source.
* @param source the source providing the DER or PEM encoded key
* @return new public key object
*/
BOTAN_PUBLIC_API(3, 0) std::unique_ptr<Public_Key> load_key(DataSource& source);

#if defined(BOTAN_TARGET_OS_HAS_FILESYSTEM)
/**
* Create a public key from a file
* @param filename pathname to the file to load
* @return new public key object
*/
inline std::unique_ptr<Public_Key> load_key(std::string_view filename) {
   DataSource_Stream source(filename, true);
   return X509::load_key(source);
}
#endif

/**
* Create a public key from a memory region.
* @param enc the memory region containing the DER or PEM encoded key
* @return new public key object
*/
inline std::unique_ptr<Public_Key> load_key(const std::vector<uint8_t>& enc) {
   DataSource_Memory source(enc);
   return X509::load_key(source);
}

/**
* Create a public key from a memory region.
* @param enc the memory region containing the DER or PEM encoded key
* @return new public key object
*/
inline std::unique_ptr<Public_Key> load_key(std::span<const uint8_t> enc) {
   DataSource_Memory source(enc);
   return X509::load_key(source);
}

/**
* Copy a key.
* @param key the public key to copy
* @return new public key object
*/
inline std::unique_ptr<Public_Key> copy_key(const Public_Key& key) {
   DataSource_Memory source(PEM_encode(key));
   return X509::load_key(source);
}

}  // namespace Botan::X509

#endif
