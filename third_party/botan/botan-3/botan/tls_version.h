/*
* TLS Protocol Version Management
* (C) 2012 Jack Lloyd
*     2021 Elektrobit Automotive GmbH
*     2022 René Meusel, Hannes Rantzsch - neXenio GmbH
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_PROTOCOL_VERSION_H_
#define BOTAN_TLS_PROTOCOL_VERSION_H_

#include <botan/types.h>
#include <string>

namespace Botan::TLS {

enum class Version_Code : uint16_t {
   TLS_V11 = 0x0302,  // not supported by Botan
   TLS_V12 = 0x0303,
   TLS_V13 = 0x0304,
   DTLS_V12 = 0xFEFD,
   DTLS_V13 = 0xFEFC,  // not supported by Botan
};

/**
* TLS Protocol Version
*/
class BOTAN_PUBLIC_API(2, 0) Protocol_Version final {
   public:
      using enum Version_Code;

      /**
      * @return latest known TLS version
      */
      static Protocol_Version latest_tls_version() {
#if defined(BOTAN_HAS_TLS_13)
         return Protocol_Version(TLS_V13);
#else
         return Protocol_Version(TLS_V12);
#endif
      }

      /**
      * @return latest known DTLS version
      */
      static Protocol_Version latest_dtls_version() { return Protocol_Version(DTLS_V12); }

      Protocol_Version() : m_version(0) {}

      explicit Protocol_Version(uint16_t code) : m_version(code) {}

      /**
      * @param named_version a specific named version of the protocol
      */
      Protocol_Version(Version_Code named_version) : Protocol_Version(static_cast<uint16_t>(named_version)) {}

      /**
      * @param major the major version
      * @param minor the minor version
      */
      Protocol_Version(uint8_t major, uint8_t minor) :
            Protocol_Version(static_cast<uint16_t>((static_cast<uint16_t>(major) << 8) | minor)) {}

      /**
      * @return true if this is a valid protocol version
      */
      bool valid() const;

      /**
      * @return true if this is a protocol version we know about
      */
      bool known_version() const;

      /**
      * @return major version of the protocol version
      */
      uint8_t major_version() const { return static_cast<uint8_t>(m_version >> 8); }

      /**
      * @return minor version of the protocol version
      */
      uint8_t minor_version() const { return static_cast<uint8_t>(m_version & 0xFF); }

      /**
      * @return the version code
      */
      uint16_t version_code() const { return m_version; }

      /**
      * @return human-readable description of this version
      */
      std::string to_string() const;

      /**
      * @return true iff this is a DTLS version
      */
      bool is_datagram_protocol() const;

      /**
       * @return true if this version indicates (D)TLS 1.2 or older
       */
      bool is_pre_tls_13() const;

      /**
       * @return true if this version indicates a (D)TLS newer than 1.3
       */
      bool is_tls_13_or_later() const;

      /**
      * @return if this version is equal to other
      */
      bool operator==(const Protocol_Version& other) const { return (m_version == other.m_version); }

      /**
      * @return if this version is not equal to other
      */
      bool operator!=(const Protocol_Version& other) const { return (m_version != other.m_version); }

      /**
      * @return if this version is later than other
      */
      bool operator>(const Protocol_Version& other) const;

      /**
      * @return if this version is later than or equal to other
      */
      bool operator>=(const Protocol_Version& other) const { return (*this == other || *this > other); }

      /**
      * @return if this version is earlier to other
      */
      bool operator<(const Protocol_Version& other) const { return !(*this >= other); }

      /**
      * @return if this version is earlier than or equal to other
      */
      bool operator<=(const Protocol_Version& other) const { return (*this == other || *this < other); }

   private:
      uint16_t m_version;
};

}  // namespace Botan::TLS

#endif
