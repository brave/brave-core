#ifndef BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_BUTTON_MANAGER_FACTORY_H_
#define BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_BUTTON_MANAGER_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace content {
class BrowserContext;
}

namespace ai_rewriter {

class AIRewriterButtonManager;

class AIRewriterButtonManagerFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  AIRewriterButtonManagerFactory(const AIRewriterButtonManagerFactory&) =
      delete;
  AIRewriterButtonManagerFactory& operator=(
      const AIRewriterButtonManagerFactory&) = delete;

  static AIRewriterButtonManager* GetForContext(
      content::BrowserContext* context);
  static AIRewriterButtonManagerFactory* GetInstance();

 private:
  friend base::NoDestructor<AIRewriterButtonManagerFactory>;

  AIRewriterButtonManagerFactory();
  ~AIRewriterButtonManagerFactory();

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};
}  // namespace ai_rewriter
#endif  // BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_BUTTON_MANAGER_FACTORY_H_
