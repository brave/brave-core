// This file contains code that used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

// Need to include registry.h because it has an `Initialize` method, and we
// define a macro with the same name below.
#include "base/win/registry.h"
#include "chrome/installer/mini_installer/configuration.h"

#define Initialize() Initialize(::GetModuleHandle(nullptr))

#include <chrome/installer/mini_installer/configuration_test.cc>

#undef Initialize
