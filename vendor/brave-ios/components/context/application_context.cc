//
//  application_context.cpp
//  Sources
//
//  Created by brandon on 2020-05-12.
//

#include "brave/vendor/brave-ios/components/context/application_context.h"

#include "base/logging.h"

namespace brave {
ApplicationContext* g_application_context = nullptr;

ApplicationContext* GetApplicationContext() {
  return g_application_context;
}

ApplicationContext::ApplicationContext() {
}

ApplicationContext::~ApplicationContext() {
}

// static
void ApplicationContext::SetApplicationContext(ApplicationContext* context) {
  g_application_context = context;
}
}
