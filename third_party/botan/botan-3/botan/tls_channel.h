/*
* TLS Channel
* (C) 2011,2012,2014,2015 Jack Lloyd
*     2016 Matthias Gierlings
*     2021 Elektrobit Automotive GmbH
*     2022 René Meusel, Hannes Rantzsch - neXenio GmbH
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_CHANNEL_H_
#define BOTAN_TLS_CHANNEL_H_

#include <botan/tls_alert.h>
#include <botan/tls_callbacks.h>
#include <botan/tls_session.h>
#include <botan/tls_session_manager.h>
#include <botan/x509cert.h>

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Botan::TLS {

/**
* Generic interface for TLS endpoint
*/
class BOTAN_PUBLIC_API(2, 0) Channel {
   public:
      static constexpr size_t IO_BUF_DEFAULT_SIZE = 10 * 1024;

      virtual ~Channel() = default;

   protected:
      virtual size_t from_peer(std::span<const uint8_t> data) = 0;
      virtual void to_peer(std::span<const uint8_t> data) = 0;

   public:
      /**
      * Inject TLS traffic received from counterparty
      * @return a hint as to how many more bytes we need to process the
      *         current record (this may be 0 if on a record boundary)
      */
      size_t received_data(std::span<const uint8_t> data) { return this->from_peer(data); }

      size_t received_data(const uint8_t buf[], size_t buf_size) { return this->from_peer(std::span(buf, buf_size)); }

      /**
      * Inject plaintext intended for counterparty
      * Throws an exception if is_active() is false
      */
      void send(std::span<const uint8_t> data) { this->to_peer(data); }

      void send(const uint8_t buf[], size_t buf_size) { this->to_peer(std::span(buf, buf_size)); }

      /**
      * Inject plaintext intended for counterparty
      * Throws an exception if is_active() is false
      */
      void send(std::string_view val) { this->send(std::span(cast_char_ptr_to_uint8(val.data()), val.size())); }

      /**
      * Inject plaintext intended for counterparty
      * Throws an exception if is_active() is false
      */

      /**
      * Send a TLS alert message. If the alert is fatal, the internal
      * state (keys, etc) will be reset.
      * @param alert the Alert to send
      */
      virtual void send_alert(const Alert& alert) = 0;

      /**
      * Send a warning alert
      */
      virtual void send_warning_alert(Alert::Type type) = 0;

      /**
      * Send a fatal alert
      */
      virtual void send_fatal_alert(Alert::Type type) = 0;

      /**
      * Send a close notification alert
      */
      virtual void close() = 0;

      /**
      * Becomes true as soon as the TLS handshake is fully complete and all
      * security assurances TLS provides can be guaranteed.
      *
      * @returns true once the TLS handshake has finished successfully
      */
      virtual bool is_handshake_complete() const = 0;

      /**
      * Check whether the connection is ready to send application data. Note
      * that a TLS 1.3 server MAY send data _before_ receiving the client's
      * Finished message. Only _after_ receiving the client's Finished, can the
      * server be sure about the client's liveness and (optional) identity.
      *
      * Consider using is_handshake_complete() if you need to wait until the
      * handshake if fully complete.
      *
      * @return true iff the connection is active for sending application data
      */
      virtual bool is_active() const = 0;

      /**
      * Note: For TLS 1.3 a connection is closed only after both peers have
      *       signaled a "close_notify". While TLS 1.2 automatically responded
      *       in suit once the peer had sent "close_notify", TLS 1.3 allows to
      *       continue transmitting data even if the peer closed their writing
      *       end.
      *
      * @return true iff the connection has been definitely closed
      */
      virtual bool is_closed() const = 0;

      /**
      * @return true iff the peer closed their channel
      *         (i.e. no more incoming data expected)
      */
      virtual bool is_closed_for_reading() const = 0;

      /**
      * @return true iff we closed our channel
      *         (i.e. no more outgoing data allowed)
      */
      virtual bool is_closed_for_writing() const = 0;

      /**
      * @return certificate chain of the peer (may be empty)
      */
      virtual std::vector<X509_Certificate> peer_cert_chain() const = 0;

      /**
      * @return raw public key of the peer (may be nullptr)
      */
      virtual std::shared_ptr<const Public_Key> peer_raw_public_key() const = 0;

      /**
       * @return identity of the PSK used for this connection
       *         or std::nullopt if no PSK was used.
       */
      virtual std::optional<std::string> external_psk_identity() const = 0;

      /**
      * Key material export (RFC 5705)
      * @param label a disambiguating label string
      * @param context a per-association context value
      * @param length the length of the desired key in bytes
      * @return key of length bytes
      */
      virtual SymmetricKey key_material_export(std::string_view label,
                                               std::string_view context,
                                               size_t length) const = 0;

      /**
      * Attempt to renegotiate the session
      * @param force_full_renegotiation if true, require a full renegotiation,
      * otherwise allow session resumption
      */
      virtual void renegotiate(bool force_full_renegotiation = false) = 0;

      /**
      * Attempt to update the session's traffic key material
      * Note that this is possible with a TLS 1.3 channel, only.
      *
      * @param request_peer_update if true, require a reciprocal key update
      */
      virtual void update_traffic_keys(bool request_peer_update = false) = 0;

      /**
      * @return true iff the counterparty supports the secure
      * renegotiation extensions.
      */
      virtual bool secure_renegotiation_supported() const = 0;

      /**
      * Perform a handshake timeout check. This does nothing unless
      * this is a DTLS channel with a pending handshake state, in
      * which case we check for timeout and potentially retransmit
      * handshake packets.
      */
      virtual bool timeout_check() = 0;

      virtual std::string application_protocol() const = 0;
};
}  // namespace Botan::TLS

#endif
