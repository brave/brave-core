/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_BROWSER_BRAVE_IPFS_CLIENT_UPDATER_H_
#define BRAVE_COMPONENTS_IPFS_BROWSER_BRAVE_IPFS_CLIENT_UPDATER_H_

#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

class BraveIpfsClientUpdaterTest;

using brave_component_updater::BraveComponent;

namespace ipfs {

#if defined(OS_WIN)
const std::string kIpfsClientComponentName("Brave Ipfs Client Updater (Windows)");
const std::string kIpfsClientComponentId("client-component");
const std::string kIpfsClientComponentBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuLxVDZm1QIzpMUFMBYym"
    "zriJGzgRYWpun1n9Qgd0240h9zchyZenLnZG0d3XLk38J+tHCoObb+o5sNuSzx43"
    "f0kb3mNk8AkZd/zc8jo9bK56Ep6E1iuWHfjDkl7mCD+o+CNAmDWgdGdaaaRiBIWL"
    "m8DXskaT0EWFVlBQK6PA0patY6IJ9AHeahRcQDMz11b4DZmCK46Yy0lWquAKpHdW"
    "5WFfljFxICOKeb7S/a1I0lWu2Y4Yv/ohbzktjcpAluefz6mE5d/sSBdQGdJzJIdo"
    "/CRfYgax5nMumx0x38CmVN53GVB+5TM0mw1bhU52ASysgZjAC0++Kbl1qXeSZuWM"
    "/QIDAQAB";

#elif defined(OS_MACOSX)
const std::string kIpfsClientComponentName("Brave Ipfs Client Updater (Mac)");
const std::string kIpfsClientComponentId("client-component");
const std::string kIpfsClientComponentBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu4gvE67b2T0U0i5awN5Q"
    "8YTyEXkahVAIaDqZaC2GkyjqJxkfThTxNq+MfjRAfeoxdKq95XATHMQPw6bHBCRr"
    "eDokesk0Yf4/2Tm+Sx+5ndrVb44bu0Qe/TM2EprxKvwyMo55pOjoHvnyhExd3E1p"
    "IS4Gq1i3LCPdkG7re+qAr2L69KyihiPzobjH50ZbjKhjIf/2P2ox5mXoZ+OpbNfu"
    "ryEr9a5YL0h4vkBF2x9qSEErNj/ksDAcvKS1S+GjKVwYzJpzRG5mgWlpaqXNRIYY"
    "59uo1UEJYwr+HQ0pvt/gEdns1ccUsGEm9PAMJRptvrGX/fauIMAASvByMRG7XC27"
    "gwIDAQAB";

#elif defined(OS_LINUX)
const std::string kIpfsClientComponentName("Brave Ipfs Client Updater (Linux)");
const std::string kIpfsClientComponentId("client-component");
const std::string kIpfsClientComponentBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuG1akBG8ka37Pdx0F21r"
    "J2efimrZnN8PrBlUBHYZ3sOBFLjzdw7gxpqznbOzNzx4hAAUXdLWPxBZXgOGV+rw"
    "MmCdskXr6dK5yLtJNjWqDHNVxyikQlIKRK3VKO9f6HZBC3SwF/GqLenuFuVxbg1q"
    "mvKkBgTUiaDb1pgqJ78/1L21gsT4RE/PO4bvU2XEg9Xr4FFLfQDemhIhXqszqmKR"
    "J9HIuxTzVft5v5Ys0S0Kqorn2xo+lFpVzZT7sV2orDHaLiVB5uqCMWhXehVixfRp"
    "BuPGdwSuzJsNkV5aGOObKfoLr1zUgstJYMLB0uWNXTfuKM4EibWUMLMqlCYVzs2R"
    "ewIDAQAB";
#endif

class BraveIpfsClientUpdater : public BraveComponent {
 public:
   BraveIpfsClientUpdater(BraveComponent::Delegate* delegate);
   ~BraveIpfsClientUpdater() override;

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
  friend class ::BraveIpfsClientUpdaterTest;
  static std::string g_ipfs_client_component_name_;
  static std::string g_ipfs_client_component_id_;
  static std::string g_ipfs_client_component_base64_public_key_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);
  void InitExecutablePath(const base::FilePath& install_dir);
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool registered_;
  base::FilePath executable_path_;

  DISALLOW_COPY_AND_ASSIGN(BraveIpfsClientUpdater);
};

// Creates the BraveIpfsClientUpdater
std::unique_ptr<BraveIpfsClientUpdater>
BraveIpfsClientUpdaterFactory(BraveComponent::Delegate* delegate);

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_BROWSER_BRAVE_IPFS_CLIENT_UPDATER_H_
