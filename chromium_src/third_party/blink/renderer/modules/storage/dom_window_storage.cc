/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/modules/storage/dom_window_storage.cc"

#include "net/base/features.h"
#include "third_party/blink/public/common/dom_storage/session_storage_namespace_id.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/modules/storage/brave_dom_window_storage.h"

namespace blink {

namespace {

constexpr char kSessionStorageSuffix[] = "/ephemeral-session-storage";

// This replicates the conversion of a string into a session storage namespace
// id that is found in the implementation of EphemeralStorageTabHelper.
String StringToSessionStorageId(const String& string,
                                const std::string& suffix) {
  std::string hash =
      base::MD5String(std::string(string.Utf8()) + suffix) + "____";
  DCHECK_EQ(hash.length(), kSessionStorageNamespaceIdLength);
  return String(hash.c_str());
}

}  // namespace

const SecurityOrigin* GetEphemeralStorageOrigin(LocalDOMWindow* window) {
  auto* frame = window->GetFrame();
  if (!frame)
    return nullptr;

  if (auto* settings_client = frame->GetContentSettingsClient())
    return settings_client->GetEphemeralStorageOriginSync().Get();

  return nullptr;
}

// EphemeralSessionStorageNamespace manages ephemeral sessionStorage namespace
// for a particular Page object. The namespace is instantiated on the Page
// lazily, as soon as a third-party frame needs ephemeral sessionStorage. It's
// then shared by all third-party frames that are embedded in this Page.
//
// The namespace is created in the browser process ahead of time. We ensure
// that we are using the same namespace by using a common naming scheme.
class EphemeralSessionStorageNamespace
    : public GarbageCollected<EphemeralSessionStorageNamespace>,
      public Supplement<Page> {
 public:
  EphemeralSessionStorageNamespace(StorageController* controller,
                                   const String& session_storage_id);
  virtual ~EphemeralSessionStorageNamespace() = default;

  static const char kSupplementName[];
  static EphemeralSessionStorageNamespace* From(Page* page,
                                                LocalDOMWindow* window);

  StorageNamespace* session_storage() { return session_storage_.Get(); }
  StorageNamespace* local_storage() { return local_storage_.Get(); }
  void Trace(Visitor* visitor) const override;

 private:
  Member<StorageNamespace> session_storage_;
  Member<StorageNamespace> local_storage_;
};

const char EphemeralSessionStorageNamespace::kSupplementName[] =
    "EphemeralSessionStorageNamespace";

EphemeralSessionStorageNamespace::EphemeralSessionStorageNamespace(
    StorageController* controller,
    const String& session_storage_id)
    : Supplement(nullptr),
      session_storage_(
          MakeGarbageCollected<StorageNamespace>(controller,
                                                 session_storage_id)),
      local_storage_(MakeGarbageCollected<StorageNamespace>(controller)) {}

void EphemeralSessionStorageNamespace::Trace(Visitor* visitor) const {
  visitor->Trace(session_storage_);
  visitor->Trace(local_storage_);
  Supplement<Page>::Trace(visitor);
}

// static
EphemeralSessionStorageNamespace* EphemeralSessionStorageNamespace::From(
    Page* page,
    LocalDOMWindow* window) {
  DCHECK(window);
  if (!page)
    return nullptr;

  EphemeralSessionStorageNamespace* supplement =
      Supplement<Page>::From<EphemeralSessionStorageNamespace>(page);
  if (supplement)
    return supplement;

  auto* web_frame = WebLocalFrameImpl::FromFrame(window->GetFrame());
  WebViewImpl* webview = web_frame->ViewImpl();
  if (!webview)
    return nullptr;
  String session_storage_id = StringToSessionStorageId(
      String::FromUTF8(webview->GetSessionStorageNamespaceId()),
      kSessionStorageSuffix);

  supplement = MakeGarbageCollected<EphemeralSessionStorageNamespace>(
      StorageController::GetInstance(), session_storage_id);

  ProvideTo(*page, supplement);
  return supplement;
}

// static
const char BraveDOMWindowStorage::kSupplementName[] = "BraveDOMWindowStorage";

// static
BraveDOMWindowStorage& BraveDOMWindowStorage::From(LocalDOMWindow& window) {
  BraveDOMWindowStorage* supplement =
      Supplement<LocalDOMWindow>::From<BraveDOMWindowStorage>(window);
  if (!supplement) {
    supplement = MakeGarbageCollected<BraveDOMWindowStorage>(window);
    ProvideTo(window, supplement);
  }
  return *supplement;
}

// static
StorageArea* BraveDOMWindowStorage::sessionStorage(
    LocalDOMWindow& window,
    ExceptionState& exception_state) {
  return From(window).sessionStorage(exception_state);
}

// static
StorageArea* BraveDOMWindowStorage::localStorage(
    LocalDOMWindow& window,
    ExceptionState& exception_state) {
  return From(window).localStorage(exception_state);
}

BraveDOMWindowStorage::BraveDOMWindowStorage(LocalDOMWindow& window)
    : Supplement<LocalDOMWindow>(window) {}

StorageArea* BraveDOMWindowStorage::sessionStorage(
    ExceptionState& exception_state) {
  LocalDOMWindow* window = GetSupplementable();
  auto* storage =
      DOMWindowStorage::From(*window).sessionStorage(exception_state);

  if (!GetEphemeralStorageOrigin(window))
    return storage;

  return ephemeralSessionStorage();
}

StorageArea* BraveDOMWindowStorage::ephemeralSessionStorage() {
  if (ephemeral_session_storage_)
    return ephemeral_session_storage_;

  LocalDOMWindow* window = GetSupplementable();
  Page* page = window->GetFrame()->GetDocument()->GetPage();
  EphemeralSessionStorageNamespace* ephemeral_namespace =
      EphemeralSessionStorageNamespace::From(page, window);
  if (!ephemeral_namespace)
    return nullptr;

  auto storage_area =
      ephemeral_namespace->session_storage()->GetCachedArea(window);

  ephemeral_session_storage_ =
      StorageArea::Create(window, std::move(storage_area),
                          StorageArea::StorageType::kSessionStorage);
  return ephemeral_session_storage_;
}

StorageArea* BraveDOMWindowStorage::localStorage(
    ExceptionState& exception_state) {
  if (ephemeral_local_storage_)
    return ephemeral_local_storage_;

  LocalDOMWindow* window = GetSupplementable();
  const SecurityOrigin* ephemeral_storage_origin =
      GetEphemeralStorageOrigin(window);
  if (ephemeral_storage_origin) {
    if (window->GetEphemeralStorageOrigin() != ephemeral_storage_origin) {
      window->SetEphemeralStorageOrigin(ephemeral_storage_origin);
    }

    Page* page = window->GetFrame()->GetDocument()->GetPage();
    EphemeralSessionStorageNamespace* ephemeral_namespace =
        EphemeralSessionStorageNamespace::From(page, window);
    if (!ephemeral_namespace)
      return nullptr;

    auto storage_area =
        ephemeral_namespace->local_storage()->GetCachedArea(window);
    ephemeral_local_storage_ =
        StorageArea::Create(window, std::move(storage_area),
                            StorageArea::StorageType::kLocalStorage);
    return ephemeral_local_storage_;
  }

  return DOMWindowStorage::From(*window).localStorage(exception_state);
}

void BraveDOMWindowStorage::Trace(Visitor* visitor) const {
  visitor->Trace(ephemeral_session_storage_);
  visitor->Trace(ephemeral_local_storage_);
  Supplement<LocalDOMWindow>::Trace(visitor);
}

}  // namespace blink
