// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/browser/ui/webui/settings/brave_extensions_manifest_v2_handler.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/extensions/chrome_content_verifier_delegate.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/crx_file/crx_verifier.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/crx_file_info.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_dialog_auto_confirm.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"

namespace {

constexpr char kExtensionId[] = "bgkmgpgeempochogfoddiobpbhdfgkdi";  // NoScript

bool ClickExtensionToggle(content::WebContents* web_contents) {
  return EvalJs(web_contents,
                "window.testing.extensionsV2Subpage.getElementById('"
                "bgkmgpgeempochogfoddiobpbhdfgkdi').click()")
      .value.is_none();
}

bool ClickExtensionRemove(content::WebContents* web_contents) {
  return EvalJs(web_contents,
                "window.testing.extensionsV2Subpage.getElementById('"
                "bgkmgpgeempochogfoddiobpbhdfgkdi').querySelector('#"
                "bgkmgpgeempochogfoddiobpbhdfgkdi').click()")
      .value.is_none();
}

bool IsExtensionToggled(content::WebContents* web_contents) {
  return EvalJs(web_contents,
                "window.testing.extensionsV2Subpage.getElementById('"
                "bgkmgpgeempochogfoddiobpbhdfgkdi').checked")
      .value.GetBool();
}

bool IsExtensionToggleEnabled(content::WebContents* web_contents) {
  return EvalJs(web_contents,
                "!window.testing.extensionsV2Subpage.getElementById('"
                "bgkmgpgeempochogfoddiobpbhdfgkdi')."
                "disabled")
      .value.GetBool();
}

void NonBlockingDelay(base::TimeDelta delay) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
  run_loop.Run();
}

std::string GupdateResponse() {
  // For NoScript.
  return R"json(
    {
      "gupdate": {
        "app": [
          {
            "appid": "bgkmgpgeempochogfoddiobpbhdfgkdi",
            "updatecheck": {
              "codebase": "https://a.test/manifest_v2/bgkmgpgeempochogfoddiobpbhdfgkdi.crx"
            }
          }
        ]
      }
    }
  )json";
}

}  // namespace

class BraveExtensionsManifestV2BrowserTest : public InProcessBrowserTest {
 public:
  BraveExtensionsManifestV2BrowserTest() {
    feature_list_.InitAndEnableFeature(kExtensionsManifestV2);
    BraveSettingsUI::ShouldExposeElementsForTesting() = true;
  }

  ~BraveExtensionsManifestV2BrowserTest() override {
    BraveSettingsUI::ShouldExposeElementsForTesting() = false;
  }

  void SimulateInstall() {
    scoped_refptr<const extensions::Extension> extension(
        extensions::ExtensionBuilder("extension").SetID(kExtensionId).Build());
    extensions::ExtensionRegistrar::Get(browser()->profile())
        ->AddExtension(extension);
  }

  void EnableExtension(bool enable) {
    if (enable) {
      extensions::ExtensionSystem::Get(browser()->profile())
          ->extension_service()
          ->EnableExtension(kExtensionId);
    } else {
      extensions::ExtensionSystem::Get(browser()->profile())
          ->extension_service()
          ->DisableExtension(kExtensionId,
                             extensions::disable_reason::DISABLE_USER_ACTION);
    }
  }

  bool IsExtensionEnabled() {
    return extensions::ExtensionRegistry::Get(browser()->profile())
        ->enabled_extensions()
        .Contains(kExtensionId);
  }

  bool IsExtensionInstalled() {
    return extensions::ExtensionRegistry::Get(browser()->profile())
        ->GetInstalledExtension(kExtensionId);
  }

  void WaitExtensionToggled(bool toggled) {
    while (IsExtensionToggled(
               browser()->tab_strip_model()->GetActiveWebContents()) !=
           toggled) {
      NonBlockingDelay(base::Milliseconds(10));
    }
  }

  void WaitExtensionInstalled() {
    while (!IsExtensionInstalled()) {
      NonBlockingDelay(base::Milliseconds(10));
    }
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveExtensionsManifestV2BrowserTest, InstallFail) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("brave://settings/extensions/v2")));
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  content::WebContentsConsoleObserver console_observer(web_contents);
  console_observer.SetPattern("Failed to download extension.");
  ClickExtensionToggle(web_contents);
  ASSERT_TRUE(console_observer.Wait());

  while (IsExtensionToggled(web_contents)) {
    NonBlockingDelay(base::Milliseconds(10));
  }
  EXPECT_TRUE(IsExtensionToggleEnabled(web_contents));
}

IN_PROC_BROWSER_TEST_F(BraveExtensionsManifestV2BrowserTest,
                       InstallDisableEnableUninstall) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("brave://settings/extensions/v2")));
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_FALSE(IsExtensionToggled(web_contents));
  EXPECT_TRUE(IsExtensionToggleEnabled(web_contents));

  SimulateInstall();
  WaitExtensionToggled(true);
  {
    // toggle extension disabled->enabled->disabled
    EnableExtension(false);
    WaitExtensionToggled(false);

    EXPECT_TRUE(IsExtensionInstalled());

    EnableExtension(true);
    WaitExtensionToggled(true);
    EXPECT_TRUE(IsExtensionInstalled());

    EnableExtension(false);
    WaitExtensionToggled(false);
  }

  // enable from settings.
  ClickExtensionToggle(web_contents);
  WaitExtensionToggled(true);
  EXPECT_TRUE(IsExtensionInstalled());
  EXPECT_TRUE(IsExtensionEnabled());

  // disabled from settings.
  ClickExtensionToggle(web_contents);
  WaitExtensionToggled(false);
  EXPECT_TRUE(IsExtensionInstalled());
  EXPECT_FALSE(IsExtensionEnabled());

  // remove from settings.
  ClickExtensionRemove(web_contents);
  EXPECT_FALSE(IsExtensionInstalled());
  EXPECT_FALSE(IsExtensionEnabled());
}

class BraveExtensionsManifestV2InstallerBrowserTest
    : public BraveExtensionsManifestV2BrowserTest {
 public:
  BraveExtensionsManifestV2InstallerBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  ~BraveExtensionsManifestV2InstallerBrowserTest() override = default;

  void SetUp() override {
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    ASSERT_TRUE(https_server_.InitializeAndListen());

    extensions::ChromeContentVerifierDelegate::SetDefaultModeForTesting(
        extensions::ChromeContentVerifierDelegate::VerifyInfo::Mode::
            ENFORCE_STRICT);
    BraveExtensionsManifestV2BrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    auto get_extension = [](const net::test_server::HttpRequest& request)
        -> std::unique_ptr<net::test_server::HttpResponse> {
      if (request.GetURL().path_piece() != "/extensions") {
        return nullptr;
      }

      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(GupdateResponse());
      return http_response;
    };

    https_server_.RegisterDefaultHandler(base::BindRepeating(get_extension));
    https_server_.ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    https_server_.StartAcceptingConnections();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    BraveExtensionsManifestV2BrowserTest::SetUpCommandLine(command_line);
    command_line->RemoveSwitch(switches::kComponentUpdater);
    command_line->AppendSwitchASCII(switches::kComponentUpdater,
                                    "url-source=https://a.test/extensions");
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP *:443 " + https_server_.host_port_pair().ToString());
  }

  void TearDown() override {
    extensions::ChromeContentVerifierDelegate::SetDefaultModeForTesting(
        std::nullopt);
    BraveExtensionsManifestV2BrowserTest::TearDown();
  }

 protected:
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveExtensionsManifestV2InstallerBrowserTest,
                       InstallExtension) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("brave://settings/extensions/v2")));
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_FALSE(IsExtensionToggled(web_contents));
  EXPECT_TRUE(IsExtensionToggleEnabled(web_contents));

  extensions::ScopedTestDialogAutoConfirm confirm(
      extensions::ScopedTestDialogAutoConfirm::ACCEPT);

  // enable from settings.
  ClickExtensionToggle(web_contents);

  WaitExtensionToggled(true);
  WaitExtensionInstalled();

  EXPECT_TRUE(IsExtensionInstalled());
  EXPECT_TRUE(IsExtensionEnabled());
}

IN_PROC_BROWSER_TEST_F(BraveExtensionsManifestV2InstallerBrowserTest,
                       InstallCancelExtension) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("brave://settings/extensions/v2")));
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_FALSE(IsExtensionToggled(web_contents));
  EXPECT_TRUE(IsExtensionToggleEnabled(web_contents));

  extensions::ScopedTestDialogAutoConfirm confirm(
      extensions::ScopedTestDialogAutoConfirm::CANCEL);

  // enable from settings.
  ClickExtensionToggle(web_contents);

  WaitExtensionToggled(false);
  EXPECT_FALSE(IsExtensionInstalled());
  EXPECT_FALSE(IsExtensionEnabled());

  auto* extension =
      extensions::ExtensionRegistry::Get(browser()->profile())
          ->GetExtensionById(kExtensionId,
                             extensions::ExtensionRegistry::EVERYTHING);
  EXPECT_FALSE(extension);
}

IN_PROC_BROWSER_TEST_F(BraveExtensionsManifestV2InstallerBrowserTest,
                       ExtensionWorks) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  constexpr char kTestExtension[] =
      "eedcldngdlcmkjdcdlffmjhpbfdcmkce";  // test/data/manifest_v2

  base::FilePath test_extension =
      base::PathService::CheckedGet(brave::DIR_TEST_DATA);
  test_extension = test_extension.AppendASCII(
      "manifest_v2/eedcldngdlcmkjdcdlffmjhpbfdcmkce.crx");

  auto installer = extensions::CrxInstaller::CreateSilent(browser()->profile());
  installer->set_allow_silent_install(true);
  installer->set_is_gallery_install(true);

  base::RunLoop run_loop;
  installer->AddInstallerCallback(base::BindLambdaForTesting(
      [&run_loop](const std::optional<extensions::CrxInstallError>& result) {
        EXPECT_FALSE(result.has_value());
        run_loop.Quit();
      }));

  ui_test_utils::TabAddedWaiter tab_waiter(browser());

  extensions::CRXFileInfo crx;
  crx.path = test_extension;
  crx.required_format = crx_file::VerifierFormat::CRX3;
  installer->InstallCrxFile(crx);

  run_loop.Run();

  content::WebContents* web_contents = tab_waiter.Wait();
  content::WaitForLoadStop(web_contents);
  EXPECT_EQ(u"Extension v2", web_contents->GetTitle());

  const auto* extension =
      extensions::ExtensionRegistry::Get(browser()->profile())
          ->GetInstalledExtension(kTestExtension);
  EXPECT_TRUE(extension->from_webstore());
  EXPECT_TRUE(base::PathExists(extension->path()
                                   .AppendASCII("_metadata")
                                   .AppendASCII("verified_contents.json")));
  EXPECT_TRUE(base::PathExists(extension->path()
                                   .AppendASCII("_metadata")
                                   .AppendASCII("computed_hashes.json")));
}
