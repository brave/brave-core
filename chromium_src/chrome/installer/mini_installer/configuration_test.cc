// Need to include registry.h because it has an `Initialize` method, and we
// define a macro with the same name below.
#include "base/win/registry.h"
#include "chrome/installer/mini_installer/configuration.h"

#define Initialize() Initialize(::GetModuleHandle(nullptr))

#include <chrome/installer/mini_installer/configuration_test.cc>

#undef Initialize
