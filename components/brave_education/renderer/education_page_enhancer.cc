// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_education/renderer/education_page_enhancer.h"

#include <array>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_education/renderer/js_api_builder.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace brave_education {

namespace {

static const std::array kBraveSiteAllowedPaths{"/whats-new"};

static const char16_t kEnhancePageScript[] =
    uR"js({

  const actionHandlers = new Map()

  actionHandlers.set('show-settings-page', (action) => {
    const url = String(action.url || '')
    const target = String(action.target || '')
    braveEducation.showSettingsPage(url, target)
  })

  function getActionData(elem) {
    const value = elem.dataset.braveEducationAction
    let data

    try {
      data = JSON.parse(value)
    } catch {
      console.warn(
          'Attribute [data-brave-education-action] contains invalid JSON')
      console.warn('Attribute value: ', value)
      return null
    }

    if (!data) {
      return null
    }

    if (!data.type || typeof data.type !== 'string') {
      console.warn('Missing "type" property on [data-brave-education-action]')
      console.warn('Attribute data: ', data)
      return null
    }

    if (!actionHandlers.has(data.type)) {
      return null
    }

    return data
  }

  function activateElements() {
    for (const elem of document.querySelectorAll('.brave-education-ui')) {
      const action = getActionData(elem)
      if (action) {
        const trigger = action.trigger
            ? elem.querySelector(action.trigger)
            : elem
        if (trigger) {
          trigger.addEventListener('click', (event) => {
            event.preventDefault();
            actionHandlers.get(action.type)(action)
          })
        }
        elem.classList.add('brave-education-active')
      }
    }
  }

  document.addEventListener('brave-education-content-ready', activateElements)

})js";

bool ShouldEnhancePage(content::RenderFrame* render_frame) {
  if (!render_frame || !render_frame->IsMainFrame()) {
    return false;
  }

  auto* web_frame = render_frame->GetWebFrame();
  DCHECK(web_frame);
  if (web_frame->IsProvisional()) {
    return false;
  }

  GURL origin = url::Origin(web_frame->GetSecurityOrigin()).GetURL();
  if (!origin.is_valid() || !origin.SchemeIs(url::kHttpsScheme) ||
      origin.host() != "brave.com") {
    return false;
  }

  GURL document_url = web_frame->GetDocument().Url();
  auto path = document_url.path_piece();

  for (const char* allowed_path : kBraveSiteAllowedPaths) {
    if (path == allowed_path) {
      return true;
    }
    if (base::StartsWith(path, std::string(allowed_path) + "/")) {
      return true;
    }
  }

  return false;
}

mojom::SettingsPageTarget ParseSettingsPageTarget(const std::string& name) {
  return mojom::SettingsPageTarget::kNone;
}

}  // namespace

EducationPageEnhancer::EducationPageEnhancer(content::RenderFrame* render_frame,
                                             int32_t world_id)
    : RenderFrameObserver(render_frame), world_id_(world_id) {}

EducationPageEnhancer::~EducationPageEnhancer() = default;

void EducationPageEnhancer::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (world_id != world_id_ || context.IsEmpty()) {
    return;
  }

  if (!ShouldEnhancePage(render_frame())) {
    return;
  }

  InjectEducationRequestAPI(context);

  render_frame()->GetWebFrame()->ExecuteScriptInIsolatedWorld(
      world_id_,
      blink::WebScriptSource(blink::WebString::FromUTF16(kEnhancePageScript)),
      blink::BackForwardCacheAware::kAllow);
}

void EducationPageEnhancer::OnDestruct() {
  delete this;
}

mojo::Remote<mojom::EducationRequestHandler>&
EducationPageEnhancer::GetRequestHandler() {
  if (!request_handler_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker()->GetInterface(
        request_handler_.BindNewPipeAndPassReceiver());
  }
  CHECK(request_handler_.is_bound());
  return request_handler_;
}

void EducationPageEnhancer::InjectEducationRequestAPI(
    v8::Local<v8::Context> context) {
  JSAPIBuilder::Create(blink::MainThreadIsolate(), context)
      .SetMethod(
          "showSettingsPage",
          base::BindRepeating(&EducationPageEnhancer::ShowSettingsPageCallback,
                              weak_factory_.GetWeakPtr()))
      .SetAsObjectProperty(context->Global(), "braveEducation");
}

void EducationPageEnhancer::ShowSettingsPageCallback(
    const std::string& relative_url,
    const std::string& target) {
  GetRequestHandler()->ShowSettingsPage(relative_url,
                                        ParseSettingsPageTarget(target));
}

}  // namespace brave_education
