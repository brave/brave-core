/*
 * FrodoKEM modes and constants
 *
 * The Fellowship of the FrodoKEM:
 * (C) 2023 Jack Lloyd
 *     2023 René Meusel, Amos Treiber - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_FRODOKEM_MODE_H_
#define BOTAN_FRODOKEM_MODE_H_

#include <botan/asn1_obj.h>

#include <vector>

namespace Botan {

class BOTAN_PUBLIC_API(3, 3) FrodoKEMMode {
   public:
      enum Mode {
         FrodoKEM640_SHAKE,
         FrodoKEM976_SHAKE,
         FrodoKEM1344_SHAKE,
         eFrodoKEM640_SHAKE,
         eFrodoKEM976_SHAKE,
         eFrodoKEM1344_SHAKE,
         FrodoKEM640_AES,
         FrodoKEM976_AES,
         FrodoKEM1344_AES,
         eFrodoKEM640_AES,
         eFrodoKEM976_AES,
         eFrodoKEM1344_AES
      };

      FrodoKEMMode(Mode mode);
      explicit FrodoKEMMode(const OID& oid);
      explicit FrodoKEMMode(std::string_view str);

      OID object_identifier() const;
      std::string to_string() const;

      Mode mode() const { return m_mode; }

      bool is_ephemeral() const {
         return m_mode == eFrodoKEM640_SHAKE || m_mode == eFrodoKEM976_SHAKE || m_mode == eFrodoKEM1344_SHAKE ||
                m_mode == eFrodoKEM640_AES || m_mode == eFrodoKEM976_AES || m_mode == eFrodoKEM1344_AES;
      }

      bool is_static() const {
         return m_mode == FrodoKEM640_SHAKE || m_mode == FrodoKEM976_SHAKE || m_mode == FrodoKEM1344_SHAKE ||
                m_mode == FrodoKEM640_AES || m_mode == FrodoKEM976_AES || m_mode == FrodoKEM1344_AES;
      }

      bool is_shake() const {
         return m_mode == eFrodoKEM640_SHAKE || m_mode == eFrodoKEM976_SHAKE || m_mode == eFrodoKEM1344_SHAKE ||
                m_mode == FrodoKEM640_SHAKE || m_mode == FrodoKEM976_SHAKE || m_mode == FrodoKEM1344_SHAKE;
      }

      bool is_aes() const {
         return m_mode == eFrodoKEM640_AES || m_mode == eFrodoKEM976_AES || m_mode == eFrodoKEM1344_AES ||
                m_mode == FrodoKEM640_AES || m_mode == FrodoKEM976_AES || m_mode == FrodoKEM1344_AES;
      }

      bool is_available() const {
         return
#if defined(BOTAN_HAS_FRODOKEM_AES)
            is_aes() ||
#endif

#if defined(BOTAN_HAS_FRODOKEM_SHAKE)
            is_shake() ||
#endif

            false;
      }

      bool operator==(const FrodoKEMMode& other) const { return m_mode == other.m_mode; }

      bool operator!=(const FrodoKEMMode& other) const { return !(*this == other); }

   private:
      Mode m_mode;
};

}  // namespace Botan

#endif
