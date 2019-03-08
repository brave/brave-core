/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/render_process_host_impl.h"
#include "brave/browser/renderer_host/brave_plugin_registry_impl.h"

#define PluginRegistryImpl BravePluginRegistryImpl
#include "../../../../../content/browser/renderer_host/render_process_host_impl.cc"
#undef PluginRegistryImpl
