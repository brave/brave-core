/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_BRAVE_IPFS_CLIENT_UPDATER_H_
#define BRAVE_COMPONENTS_IPFS_BRAVE_IPFS_CLIENT_UPDATER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "components/component_updater/component_updater_service.h"

class BraveIpfsClientUpdaterTest;

using brave_component_updater::BraveComponent;

namespace ipfs {

#if defined(OS_WIN)
static const char kIpfsClientComponentName[] =
    "Brave IPFS Client Updater (Windows)";
static const char kIpfsClientComponentId[] = "lnbclahgobmjphilkalbhebakmblnbij";
static const char kIpfsClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuLxVDZm1QIzpMUFMBYym"
    "zriJGzgRYWpun1n9Qgd0240h9zchyZenLnZG0d3XLk38J+tHCoObb+o5sNuSzx43"
    "f0kb3mNk8AkZd/zc8jo9bK56Ep6E1iuWHfjDkl7mCD+o+CNAmDWgdGdaaaRiBIWL"
    "m8DXskaT0EWFVlBQK6PA0patY6IJ9AHeahRcQDMz11b4DZmCK46Yy0lWquAKpHdW"
    "5WFfljFxICOKeb7S/a1I0lWu2Y4Yv/ohbzktjcpAluefz6mE5d/sSBdQGdJzJIdo"
    "/CRfYgax5nMumx0x38CmVN53GVB+5TM0mw1bhU52ASysgZjAC0++Kbl1qXeSZuWM"
    "/QIDAQAB";
#elif defined(OS_MAC)
static const char kIpfsClientComponentName[] =
    "Brave IPFS Client Updater (Mac)";
#if defined(ARCH_CPU_ARM64)
static const char kIpfsClientComponentId[] = "lejaflgbgglfaomemffoaappaihfligf";
static const char kIpfsClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2qqbXvEZP1dlJW7FhKLB"
    "+8ZTRF4mZjxRwU9VMyPrymWhAyhurtp2eIaAY2YiFMAfg4v3Eragxlt4+fL0QETc"
    "lkmRUTvZ4wm93HODXPfL8LvKoFDBsjv9vnsT+PDonnpQBKdgRGpVYxxDY3vYu4AI"
    "KuLLY1tOGnC7XNiQWPSnagSycdQfTxdmPaiEwDde1jYcBVyIbZPkiE2F+np9jQah"
    "SKJJOKGmBaL/YO9xmjIBfPopwVVyVJPAIH6SPxI+XQMpYA1zagih5ULm+wXBNYcq"
    "Xn9W/KQPkB4HKZ0eVgcKKS6T8lwDhB2oYAaTtxRno5Fu6wlEQBGmdFxqJw8KNPu2"
    "JQIDAQAB";
#else
static const char kIpfsClientComponentId[] = "nljcddpbnaianmglkpkneakjaapinabi";
static const char kIpfsClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu4gvE67b2T0U0i5awN5Q"
    "8YTyEXkahVAIaDqZaC2GkyjqJxkfThTxNq+MfjRAfeoxdKq95XATHMQPw6bHBCRr"
    "eDokesk0Yf4/2Tm+Sx+5ndrVb44bu0Qe/TM2EprxKvwyMo55pOjoHvnyhExd3E1p"
    "IS4Gq1i3LCPdkG7re+qAr2L69KyihiPzobjH50ZbjKhjIf/2P2ox5mXoZ+OpbNfu"
    "ryEr9a5YL0h4vkBF2x9qSEErNj/ksDAcvKS1S+GjKVwYzJpzRG5mgWlpaqXNRIYY"
    "59uo1UEJYwr+HQ0pvt/gEdns1ccUsGEm9PAMJRptvrGX/fauIMAASvByMRG7XC27"
    "gwIDAQAB";
#endif
#elif defined(OS_LINUX)
static const char kIpfsClientComponentName[] =
    "Brave IPFS Client Updater (Linux)";
static const char kIpfsClientComponentId[] = "oecghfpdmkjlhnfpmmjegjacfimiafjp";
static const char kIpfsClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuG1akBG8ka37Pdx0F21r"
    "J2efimrZnN8PrBlUBHYZ3sOBFLjzdw7gxpqznbOzNzx4hAAUXdLWPxBZXgOGV+rw"
    "MmCdskXr6dK5yLtJNjWqDHNVxyikQlIKRK3VKO9f6HZBC3SwF/GqLenuFuVxbg1q"
    "mvKkBgTUiaDb1pgqJ78/1L21gsT4RE/PO4bvU2XEg9Xr4FFLfQDemhIhXqszqmKR"
    "J9HIuxTzVft5v5Ys0S0Kqorn2xo+lFpVzZT7sV2orDHaLiVB5uqCMWhXehVixfRp"
    "BuPGdwSuzJsNkV5aGOObKfoLr1zUgstJYMLB0uWNXTfuKM4EibWUMLMqlCYVzs2R"
    "ewIDAQAB";
#else
// Not used yet for Android/iOS
static const char kIpfsClientComponentName[] = "";
static const char kIpfsClientComponentId[] = "";
static const char kIpfsClientComponentBase64PublicKey[] = "";
#endif

class BraveIpfsClientUpdater : public BraveComponent,
                               public BraveComponent::ComponentObserver {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnExecutableReady(const base::FilePath& path) = 0;
    virtual void OnInstallationEvent(
        BraveComponent::ComponentObserver::Events event) = 0;

   protected:
    ~Observer() override = default;
  };

  explicit BraveIpfsClientUpdater(BraveComponent::Delegate* delegate,
                                  const base::FilePath& user_data_dir);
  BraveIpfsClientUpdater(const BraveIpfsClientUpdater&) = delete;
  BraveIpfsClientUpdater& operator=(const BraveIpfsClientUpdater&) = delete;
  ~BraveIpfsClientUpdater() override;

  void Register();
  base::FilePath GetExecutablePath() const;
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() {
    return task_runner_;
  }

  bool IsRegistered() const { return registered_; }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  void Cleanup();

 protected:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  // BraveComponent::ComponentObserver
  void OnEvent(Events event, const std::string& id) override;

 private:
  friend class ::BraveIpfsClientUpdaterTest;
  static std::string g_ipfs_client_component_name_;
  static std::string g_ipfs_client_component_id_;
  static std::string g_ipfs_client_component_base64_public_key_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);
  void SetExecutablePath(const base::FilePath& path);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool registered_;
  base::FilePath user_data_dir_;
  base::FilePath executable_path_;
  base::ObserverList<Observer> observers_;
  base::ScopedObservation<BraveComponent, BraveComponent::ComponentObserver>
      updater_observer_{this};

  base::WeakPtrFactory<BraveIpfsClientUpdater> weak_ptr_factory_;
};

// Creates the BraveIpfsClientUpdater
std::unique_ptr<BraveIpfsClientUpdater> BraveIpfsClientUpdaterFactory(
    BraveComponent::Delegate* delegate,
    const base::FilePath& user_data_dir);

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_BRAVE_IPFS_CLIENT_UPDATER_H_
