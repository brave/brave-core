#ifndef BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace brave_talk {
class BraveTalkService;

class BraveTalkServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  BraveTalkServiceFactory(const BraveTalkServiceFactory&) = delete;
  BraveTalkServiceFactory& operator=(const BraveTalkServiceFactory&) = delete;
  ~BraveTalkServiceFactory() override;

  static BraveTalkService* GetForContext(content::BrowserContext* context);
  static BraveTalkServiceFactory* GetInstance();

  KeyedService* BuildServiceInstanceFor(content::BrowserContext* context) const override;

 private:
  friend struct base::DefaultSingletonTraits<BraveTalkServiceFactory>;
  BraveTalkServiceFactory();
};
}  // namespace brave_talk

#endif // BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_FACTORY_H_