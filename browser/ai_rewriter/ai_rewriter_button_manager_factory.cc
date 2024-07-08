#include "brave/browser/ai_rewriter/ai_rewriter_button_manager_factory.h"

#include "brave/browser/ai_rewriter/ai_rewriter_button_manager.h"
#include "brave/browser/profiles/profile_util.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
namespace ai_rewriter {

// static
AIRewriterButtonManagerFactory* AIRewriterButtonManagerFactory::GetInstance() {
  static base::NoDestructor<AIRewriterButtonManagerFactory> instance;
  return instance.get();
}

AIRewriterButtonManagerFactory::AIRewriterButtonManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "AIRewriterButtonManagerFactory",
          BrowserContextDependencyManager::GetInstance()) {}

AIRewriterButtonManagerFactory::~AIRewriterButtonManagerFactory() = default;

KeyedService* AIRewriterButtonManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!brave::IsRegularProfile(context)) {
    return nullptr;
  }

  return new AIRewriterButtonManager();
}
}  // namespace ai_rewriter
