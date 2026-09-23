// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/ui/webui/custom_profile_image/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/signin/signin_view_controller_delegate.h"
#include "chrome/browser/ui/webui/signin/signin_url_utils.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/dom_key.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/shell_dialogs/fake_select_file_dialog.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace {

class FileSelectListener : public content::FileSelectListener {
 public:
  bool canceled() const { return canceled_; }

  void FileSelected(std::vector<blink::mojom::FileChooserFileInfoPtr>,
                    const base::FilePath&,
                    blink::mojom::FileChooserParams::Mode) override {
    ADD_FAILURE() << "Unexpected file selection";
  }

  void FileSelectionCanceled() override { canceled_ = true; }

 protected:
  ~FileSelectListener() override = default;

 private:
  bool canceled_ = false;
};

class ProfileCustomizationFileChooserBrowserTest : public InProcessBrowserTest {
 public:
  explicit ProfileCustomizationFileChooserBrowserTest(bool enabled) {
    feature_list_.InitWithFeatureState(
        custom_profile_image::features::kBraveCustomProfileImage, enabled);
  }

  void TearDownOnMainThread() override {
    if (delegate_) {
      auto* delegate = delegate_.get();
      delegate_ = nullptr;
      content::WebContentsDestroyedWatcher watcher(delegate->GetWebContents());
      delegate->CloseModalSignin();
      watcher.Wait();
    }
    ui::SelectFileDialog::SetFactory(nullptr);
    InProcessBrowserTest::TearDownOnMainThread();
  }

 protected:
  void CreateProfileCustomizationDialog(
      bool is_local_profile_creation,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    delegate_ =
        SigninViewControllerDelegate::CreateProfileCustomizationDelegate(
            browser(), is_local_profile_creation);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
  }

  void CreateSyncConfirmationDialog(
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    delegate_ = SigninViewControllerDelegate::CreateSyncConfirmationDelegate(
        browser(), SyncConfirmationStyle::kSigninInterceptModal,
        /*is_sync_promo=*/false);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
    // Keep the style eligible so this case isolates the origin guard.
    ASSERT_EQ(
        ProfileCustomizationStyle::kLocalProfileCreation,
        GetProfileCustomizationStyle(web_contents()->GetLastCommittedURL()));
  }

  content::WebContents* web_contents() { return delegate_->GetWebContents(); }

  void ExpectFileChooserCanceled(
      content::RenderFrameHost* frame,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    auto* factory = ui::FakeSelectFileDialog::RegisterFactory();
    factory->SetOpenCallback(base::DoNothing());
    auto listener = base::MakeRefCounted<FileSelectListener>();
    blink::mojom::FileChooserParams params;
    params.mode = blink::mojom::FileChooserParams::Mode::kOpen;
    web_contents()->GetDelegate()->RunFileChooser(frame, listener, params);
    EXPECT_TRUE(listener->canceled()) << "Ineligible request was not canceled";
    EXPECT_EQ(nullptr, factory->GetLastDialog())
        << "Ineligible request opened a native chooser";
  }

  void OpenAvatarPicker(base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    // The page world owns the custom element registry and Lit update promises.
    content::SimulateEndOfPaintHoldingOnPrimaryMainFrame(web_contents());
    ASSERT_EQ(true, content::EvalJs(web_contents(), R"(
      (async () => {
        await customElements.whenDefined('profile-customization-app');
        const app = document.querySelector('profile-customization-app');
        await app.updateComplete;
        const entered = new Promise(resolve => {
          app.shadowRoot.querySelector('#selectAvatarDialog')
              .addEventListener('view-enter-finish', resolve, {once: true});
        });
        app.shadowRoot.querySelector('#customizeAvatarIcon').click();
        await entered;
        await customElements.whenDefined('br-custom-profile-image-row');
        await app.updateComplete;
        return app.shadowRoot
            .querySelector('br-custom-profile-image-row') !== null;
      })()
    )"));
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  raw_ptr<SigninViewControllerDelegate> delegate_ = nullptr;
};

class ProfileCustomizationFileChooserEnabledBrowserTest
    : public ProfileCustomizationFileChooserBrowserTest {
 public:
  ProfileCustomizationFileChooserEnabledBrowserTest()
      : ProfileCustomizationFileChooserBrowserTest(true) {}
};

class ProfileCustomizationFileChooserDisabledBrowserTest
    : public ProfileCustomizationFileChooserBrowserTest {
 public:
  ProfileCustomizationFileChooserDisabledBrowserTest()
      : ProfileCustomizationFileChooserBrowserTest(false) {}
};

IN_PROC_BROWSER_TEST_F(ProfileCustomizationFileChooserEnabledBrowserTest,
                       KeyboardUploadOpensNativeChooser) {
  ASSERT_NO_FATAL_FAILURE(CreateProfileCustomizationDialog(true));
  ASSERT_NO_FATAL_FAILURE(OpenAvatarPicker());

  auto* factory = ui::FakeSelectFileDialog::RegisterFactory();
  base::test::TestFuture<void> opened;
  factory->SetOpenCallback(opened.GetRepeatingCallback());

  ASSERT_EQ(true, content::EvalJs(web_contents(), R"(
    (async () => {
      const app = document.querySelector('profile-customization-app');
      const row = app.shadowRoot.querySelector('br-custom-profile-image-row');
      await row.updateComplete;
      const input = row.shadowRoot.querySelector('#fileInput');
      window.fileInputCanceled = new Promise(resolve => {
        input.addEventListener('cancel', () => resolve(true), {once: true});
      });
      const button = row.shadowRoot.querySelector('#uploadButton')
          .shadowRoot.querySelector('button');
      button.focus();
      return button.getRootNode().activeElement === button;
    })()
  )"));
  content::SimulateKeyPress(web_contents(), ui::DomKey::ENTER,
                            ui::DomCode::ENTER, ui::VKEY_RETURN,
                            /*control=*/false, /*shift=*/false,
                            /*alt=*/false, /*command=*/false);
  ASSERT_TRUE(opened.Wait()) << "Upload did not open the native chooser";
  ASSERT_NE(nullptr, factory->GetLastDialog());
  factory->GetLastDialog()->CallFileSelectionCanceled();
  EXPECT_EQ(true, content::EvalJs(web_contents(), "window.fileInputCanceled"));
}

IN_PROC_BROWSER_TEST_F(ProfileCustomizationFileChooserEnabledBrowserTest,
                       SelectedFileRendersPreview) {
  ASSERT_NO_FATAL_FAILURE(CreateProfileCustomizationDialog(true));
  ASSERT_NO_FATAL_FAILURE(OpenAvatarPicker());
  auto* factory = ui::FakeSelectFileDialog::RegisterFactory();
  base::test::TestFuture<void> opened;
  factory->SetOpenCallback(opened.GetRepeatingCallback());
  ASSERT_TRUE(content::ExecJs(web_contents(), R"(
    document.querySelector('profile-customization-app').shadowRoot
        .querySelector('br-custom-profile-image-row').shadowRoot
        .querySelector('#uploadButton').click();
  )"));
  ASSERT_TRUE(opened.Wait()) << "Upload did not open the native chooser";
  ASSERT_NE(nullptr, factory->GetLastDialog());
  const auto test_data = base::PathService::CheckedGet(chrome::DIR_TEST_DATA);
  ASSERT_TRUE(factory->GetLastDialog()->CallFileSelected(
      test_data.AppendASCII("image_decoding/droids.png"), "png"));

  // Use real file bytes and decoding to cover the modal's file access and CSP.
  EXPECT_EQ(true, content::EvalJs(web_contents(), R"(
    new Promise((resolve, reject) => {
      const app = document.querySelector('profile-customization-app');
      const root = app.shadowRoot.querySelector('br-custom-profile-image-row')
          .shadowRoot;
      const observer = new MutationObserver(check);
      function check() {
        const image = root.querySelector('#previewImage');
        if (image) {
          observer.disconnect();
          image.decode().then(() => resolve(image.naturalWidth > 0), reject);
        } else if (root.querySelector('#fileError')) {
          observer.disconnect();
          reject(new Error('Selected image was rejected'));
        }
      }
      observer.observe(root, {childList: true, subtree: true});
      check();
    })
  )",
                                  content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                  ISOLATED_WORLD_ID_BRAVE_INTERNAL));
}

IN_PROC_BROWSER_TEST_F(ProfileCustomizationFileChooserEnabledBrowserTest,
                       CancelsFileChooserFromChildFrame) {
  ASSERT_NO_FATAL_FAILURE(CreateProfileCustomizationDialog(true));
  ASSERT_TRUE(content::ExecJs(web_contents(), R"(
    new Promise(resolve => {
      const frame = document.createElement('iframe');
      // The WebUI cannot be embedded; about:blank inherits its origin.
      frame.src = 'about:blank' + location.search;
      frame.addEventListener('load', resolve, {once: true});
      document.body.append(frame);
    })
  )",
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL));
  auto* frame = content::ChildFrameAt(web_contents()->GetPrimaryMainFrame(), 0);
  ASSERT_NE(nullptr, frame);
  // Keep origin and style eligible so only the primary-frame guard rejects it.
  ASSERT_EQ(web_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin(),
            frame->GetLastCommittedOrigin());
  ASSERT_EQ(ProfileCustomizationStyle::kLocalProfileCreation,
            GetProfileCustomizationStyle(frame->GetLastCommittedURL()));
  ExpectFileChooserCanceled(frame);
}

IN_PROC_BROWSER_TEST_F(ProfileCustomizationFileChooserEnabledBrowserTest,
                       CancelsFileChooserWithoutFrame) {
  ASSERT_NO_FATAL_FAILURE(CreateProfileCustomizationDialog(true));
  ExpectFileChooserCanceled(nullptr);
}

IN_PROC_BROWSER_TEST_F(ProfileCustomizationFileChooserDisabledBrowserTest,
                       CancelsFileChooser) {
  ASSERT_NO_FATAL_FAILURE(CreateProfileCustomizationDialog(true));
  ExpectFileChooserCanceled(web_contents()->GetPrimaryMainFrame());
}

IN_PROC_BROWSER_TEST_F(ProfileCustomizationFileChooserEnabledBrowserTest,
                       CancelsFileChooserOutsideProfileCustomization) {
  ASSERT_NO_FATAL_FAILURE(CreateSyncConfirmationDialog());
  ExpectFileChooserCanceled(web_contents()->GetPrimaryMainFrame());
}

IN_PROC_BROWSER_TEST_F(ProfileCustomizationFileChooserEnabledBrowserTest,
                       CancelsFileChooserForNonLocalProfile) {
  ASSERT_NO_FATAL_FAILURE(CreateProfileCustomizationDialog(false));
  ExpectFileChooserCanceled(web_contents()->GetPrimaryMainFrame());
}

}  // namespace
