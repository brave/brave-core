/*
* Alert Message
* (C) 2004-2006,2011,2012 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_ALERT_H_
#define BOTAN_TLS_ALERT_H_

#include <botan/secmem.h>
#include <string>

namespace Botan::TLS {

/**
* Type codes for TLS alerts
*
* The enumeration value matches the wire encoding
*/
enum class AlertType {
   CloseNotify = 0,
   UnexpectedMessage = 10,
   BadRecordMac = 20,
   DecryptionFailed = 21,
   RecordOverflow = 22,
   DecompressionFailure = 30,
   HandshakeFailure = 40,
   NoCertificate = 41,  // SSLv3 only
   BadCertificate = 42,
   UnsupportedCertificate = 43,
   CertificateRevoked = 44,
   CertificateExpired = 45,
   CertificateUnknown = 46,
   IllegalParameter = 47,
   UnknownCA = 48,
   AccessDenied = 49,
   DecodeError = 50,
   DecryptError = 51,
   ExportRestriction = 60,
   ProtocolVersion = 70,
   InsufficientSecurity = 71,
   InternalError = 80,
   InappropriateFallback = 86,
   UserCanceled = 90,
   NoRenegotiation = 100,
   MissingExtension = 109,  // RFC 8446
   UnsupportedExtension = 110,
   CertificateUnobtainable = 111,
   UnrecognizedName = 112,
   BadCertificateStatusResponse = 113,
   BadCertificateHashValue = 114,
   UnknownPSKIdentity = 115,
   CertificateRequired = 116,    // RFC 8446
   NoApplicationProtocol = 120,  // RFC 7301

   // pseudo alert values
   None = 256,

   // Compat enum variants, will be removed in a future major release
   CLOSE_NOTIFY BOTAN_DEPRECATED("Use CloseNotify") = CloseNotify,
   NO_APPLICATION_PROTOCOL BOTAN_DEPRECATED("Use NoApplicationProtocol") = NoApplicationProtocol,
   PROTOCOL_VERSION BOTAN_DEPRECATED("Use ProtocolVersion") = ProtocolVersion,
};

/**
* SSL/TLS Alert Message
*/
class BOTAN_PUBLIC_API(2, 0) Alert final {
   public:
      typedef AlertType Type;
      using enum AlertType;

      /**
      * @return true iff this alert is non-empty
      */
      bool is_valid() const { return (m_type_code != AlertType::None); }

      /**
      * @return if this alert is fatal or not
      *
      * Note:
      *    RFC 8446 6.
      *       In TLS 1.3, the severity is implicit in the type of alert being sent,
      *       and the "level" field can safely be ignored.
      *    Everything is considered fatal except for UserCanceled and CloseNotify (RFC 8446 6.1)
      */
      bool is_fatal() const { return m_fatal; }

      /**
      * @return type of alert
      */
      Type type() const { return m_type_code; }

      /**
      * @return type of alert
      */
      std::string type_string() const;

      /**
      * Serialize an alert
      */
      std::vector<uint8_t> serialize() const;

      /**
      * Deserialize an Alert message
      * @param buf the serialized alert
      */
      explicit Alert(const secure_vector<uint8_t>& buf);

      /**
      * Create a new Alert
      * @param type_code the type of alert
      * @param fatal specifies if this is a fatal alert
      */
      Alert(Type type_code, bool fatal = false) : m_fatal(fatal), m_type_code(type_code) {}

      Alert() : m_fatal(false), m_type_code(AlertType::None) {}

   private:
      bool m_fatal;
      Type m_type_code;
};

}  // namespace Botan::TLS

#endif
