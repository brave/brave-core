/*
* TLS Client
* (C) 2004-2011 Jack Lloyd
*     2016 Matthias Gierlings
*     2021 Elektrobit Automotive GmbH
*     2022 René Meusel, Hannes Rantzsch - neXenio GmbH
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_CLIENT_H_
#define BOTAN_TLS_CLIENT_H_

#include <botan/credentials_manager.h>
#include <botan/tls_channel.h>
#include <botan/tls_policy.h>
#include <memory>
#include <vector>

namespace Botan::TLS {

class Channel_Impl;
class Handshake_IO;

/**
* SSL/TLS Client
*/
class BOTAN_PUBLIC_API(2, 0) Client final : public Channel {
   public:
      /**
      * Set up a new TLS client session
      *
      * @param callbacks contains a set of callback function references
      *        required by the TLS client.
      *
      * @param session_manager manages session state
      *
      * @param creds manages application/user credentials
      *
      * @param policy specifies other connection policy information
      *
      * @param rng a random number generator
      *
      * @param server_info is identifying information about the TLS server
      *
      * @param offer_version specifies which version we will offer
      *        to the TLS server.
      *
      * @param next_protocols specifies protocols to advertise with ALPN
      *
      * @param reserved_io_buffer_size This many bytes of memory will
      *        be preallocated for the read and write buffers. Smaller
      *        values just mean reallocations and copies are more likely.
      */
      Client(const std::shared_ptr<Callbacks>& callbacks,
             const std::shared_ptr<Session_Manager>& session_manager,
             const std::shared_ptr<Credentials_Manager>& creds,
             const std::shared_ptr<const Policy>& policy,
             const std::shared_ptr<RandomNumberGenerator>& rng,
             Server_Information server_info = Server_Information(),
             Protocol_Version offer_version = Protocol_Version::latest_tls_version(),
             const std::vector<std::string>& next_protocols = {},
             size_t reserved_io_buffer_size = TLS::Client::IO_BUF_DEFAULT_SIZE);

      ~Client() override;

      /**
      * @return network protocol as advertised by the TLS server, if server sent the ALPN extension
      */
      std::string application_protocol() const override;

      size_t from_peer(std::span<const uint8_t> data) override;

      bool is_handshake_complete() const override;

      bool is_active() const override;

      bool is_closed() const override;

      bool is_closed_for_reading() const override;
      bool is_closed_for_writing() const override;

      std::vector<X509_Certificate> peer_cert_chain() const override;
      std::shared_ptr<const Public_Key> peer_raw_public_key() const override;
      std::optional<std::string> external_psk_identity() const override;

      SymmetricKey key_material_export(std::string_view label, std::string_view context, size_t length) const override;

      void renegotiate(bool force_full_renegotiation = false) override;

      void update_traffic_keys(bool request_peer_update = false) override;

      bool secure_renegotiation_supported() const override;

      void to_peer(std::span<const uint8_t> data) override;

      void send_alert(const Alert& alert) override;

      void send_warning_alert(Alert::Type type) override;

      void send_fatal_alert(Alert::Type type) override;

      void close() override;

      bool timeout_check() override;

   private:
      size_t downgrade();

   private:
      std::unique_ptr<Channel_Impl> m_impl;
};
}  // namespace Botan::TLS

#endif
