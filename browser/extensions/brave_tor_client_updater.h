/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_TOR_CLIENT_UPDATER_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_TOR_CLIENT_UPDATER_H_

#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

class BraveTorClientUpdaterTest;

using brave_component_updater::BraveComponent;

namespace extensions {

#if defined(OS_WIN)
const std::string kTorClientComponentName("Brave Tor Client Updater (Windows)");
const std::string kTorClientComponentId("cpoalefficncklhjfpglfiplenlpccdb");
const std::string kTorClientComponentBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1AYAsmR/VoRwkZCsjRpD"
    "58xjrgngW5y17H6BqQ7/CeNSpmXlcMXy6bJs2D/yeS96rhZSrQSHTzS9h/ieo/NZ"
    "F5PIwcv07YsG5sRd6zF5a6m92aWCQa1OkbL6jpcpL2Tbc4mCqNxhKMErT7EtIIWL"
    "9cW+mtFUjUjvV3rJLQ3Vy9u6fEi77Y8b25kGnTJoVt3uETAIHBnyNpL7ac2f8Iq+"
    "4Qa6VFmuoBhup54tTZvMv+ikoKKaQkHzkkjTa4hV5AzdnFDKO8C9qJb3T/Ef0+MO"
    "IuZjyySVzGNcOfASeHkhxhlwMQSQuhCN5mdFW5YBnVZ/5QWx8WzbhqBny/ZynS4e"
    "rQIDAQAB";
#elif defined(OS_MACOSX)
const std::string kTorClientComponentName("Brave Tor Client Updater (Mac)");
const std::string kTorClientComponentId("cldoidikboihgcjfkhdeidbpclkineef");
const std::string kTorClientComponentBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAw2QUXSbVuRxYpItYApZ8"
    "Ly/fGeUD3A+vb3J7Ot62CF32wTfWweANWyyB+EBGfbtNDAuRlAbNk0QYeCQEttuf"
    "jLh3Kd5KR5fSyyNNd2cAzAckQ8p7JdiFYjvqZLGC5vlnHgqq4O8xACX5EPwHLNFD"
    "iSpsthNmz3GCUrHrzPHjHVfy+IuucQXygnRv2fwIaAIxJmTbYm4fqsGKpfolWdMe"
    "jKVAy1hc9mApZSyt4oGvUu4SJZnxlYMrY4Ze+OWbDesi2JGy+6dA1ddL9IdnwCb3"
    "9CBOMNjaHeCVz0MKxdCWGPieQM0R7S1KvDCVqAkss6NAbLB6AVM0JulqxC9b+hr/"
    "xwIDAQAB";
#elif defined(OS_LINUX)
const std::string kTorClientComponentName("Brave Tor Client Updater (Linux)");
const std::string kTorClientComponentId("biahpgbdmdkfgndcmfiipgcebobojjkp");
const std::string kTorClientComponentBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAseuq8dXKawkZC7RSE7xb"
    "lRwh6DD+oPEGEjZWKh596/42IrWNQw60gRIR6s7x0YHh5geFnBRkx9bisEXOrFkq"
    "oArVY7eD0gMkjpor9CneD5CnCxc9/2uIPajtXfAmmLAHtN6Wk7yW30SkRf/WvLWX"
    "/H+PqskQBN7I5MO7sveYxSrRMSj7prrFHEiFmXTgG/DwjpzrA7KV6vmzz/ReD51o"
    "+UuLHE7cxPhnsNd/52uY3Lod3GhxvDoXKYx9kWlzBjxB53A2eLBCDIwwCpqS4/Ib"
    "RSJhvF33KQT8YM+7V1MitwB49klP4aEWPXwOlFHmn9Dkmlx2RbO7S0tRcH9UH4LK"
    "2QIDAQAB";
#endif

class BraveTorClientUpdater : public BraveComponent {
 public:
   BraveTorClientUpdater(BraveComponent::Delegate* delegate);
   ~BraveTorClientUpdater() override;

  void Register();
  base::FilePath GetExecutablePath() const;
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() {
    return task_runner_;
  }

 protected:
  void OnComponentReady(const std::string& component_id,
      const base::FilePath& install_dir,
      const std::string& manifest) override;

 private:
  friend class ::BraveTorClientUpdaterTest;
  static std::string g_tor_client_component_name_;
  static std::string g_tor_client_component_id_;
  static std::string g_tor_client_component_base64_public_key_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);
  void InitExecutablePath(const base::FilePath& install_dir);
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool registered_;
  base::FilePath executable_path_;

  DISALLOW_COPY_AND_ASSIGN(BraveTorClientUpdater);
};

// Creates the BraveTorClientUpdater
std::unique_ptr<BraveTorClientUpdater>
BraveTorClientUpdaterFactory(BraveComponent::Delegate* delegate);

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_TOR_CLIENT_UPDATER_H_
