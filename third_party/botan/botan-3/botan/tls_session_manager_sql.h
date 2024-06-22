/*
* TLS Session Manager storing to encrypted SQL db table
* (C) 2012,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_SQL_SESSION_MANAGER_H_
#define BOTAN_TLS_SQL_SESSION_MANAGER_H_

#include <botan/database.h>
#include <botan/tls_session_manager.h>

namespace Botan {

class RandomNumberGenerator;

namespace TLS {

/**
* An implementation of Session_Manager that saves values in a SQL
* database file, with the session data encrypted using a passphrase.
*
* @warning For clients, the hostnames associated with the saved
* sessions are stored in the database in plaintext. This may be a
* serious privacy risk in some situations.
*/
class BOTAN_PUBLIC_API(3, 0) Session_Manager_SQL : public Session_Manager {
   public:
      /**
      * @param db A connection to the database to use
               The table names botan_tls_sessions and
               botan_tls_sessions_metadata will be used
      * @param passphrase used to encrypt the session data
      * @param rng a random number generator
      * @param max_sessions a hint on the maximum number of sessions
      *        to keep in memory at any one time. (If zero, don't cap)
      */
      Session_Manager_SQL(std::shared_ptr<SQL_Database> db,
                          std::string_view passphrase,
                          const std::shared_ptr<RandomNumberGenerator>& rng,
                          size_t max_sessions = 1000);

      Session_Manager_SQL(const Session_Manager_SQL&) = delete;
      Session_Manager_SQL& operator=(const Session_Manager_SQL&) = delete;

      void store(const Session& session, const Session_Handle& handle) override;
      size_t remove(const Session_Handle& handle) override;
      size_t remove_all() override;

      bool emits_session_tickets() override { return false; }

   protected:
      std::optional<Session> retrieve_one(const Session_Handle& handle) override;
      std::vector<Session_with_Handle> find_some(const Server_Information& info, size_t max_sessions_hint) override;

      /**
       * Decides whether the underlying database is considered threadsafe in the
       * context the Session_Manager is used. If this returns `false`, accesses
       * to the database are serialized with the base class' recursive mutex.
       */
      virtual bool database_is_threadsafe() const { return m_db->is_threadsafe(); }

   private:
      // Database Schema Revision history
      //
      // 0        - empty database (needs creation with latest schema)
      // 1        - corrupted database detected (re-create it with latest schema)
      // 20120609 - older (Botan 2.0) database scheme
      // 20230113 - adapt to Botan 3.0 Session_Manager API
      //            (Session objects don't contain Session_ID, Session_Ticket)
      enum Schema_Revision {
         EMPTY = 0,
         CORRUPTED = 1,
         PRE_BOTAN_3_0 = 20120609,
         BOTAN_3_0 = 20230112,
      };

      void create_or_migrate_and_open(std::string_view passphrase);
      Schema_Revision detect_schema_revision();
      void create_with_latest_schema(std::string_view passphrase, Schema_Revision rev);
      void initialize_existing_database(std::string_view passphrase);

      void prune_session_cache();

   private:
      std::shared_ptr<SQL_Database> m_db;
      SymmetricKey m_session_key;
      size_t m_max_sessions;
};

}  // namespace TLS

}  // namespace Botan

#endif
