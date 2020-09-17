
#ifndef browser_state_keyed_service_factory_h
#define browser_state_keyed_service_factory_h

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/keyed_service/core/keyed_service_export.h"
#include "components/keyed_service/core/keyed_service_factory.h"

class KeyedService;

namespace web {
class BrowserState;
}

namespace brave {
class BrowserStateDependencyManager;

class BrowserStateKeyedServiceFactory : public KeyedServiceFactory {
 public:
  using TestingFactory = base::RepeatingCallback<std::unique_ptr<KeyedService>(
      web::BrowserState* context)>;
        
  void SetTestingFactory(web::BrowserState* context,
                         TestingFactory testing_factory);
        
  KeyedService* SetTestingFactoryAndUse(web::BrowserState* context,
                                        TestingFactory testing_factory);

 protected:
  BrowserStateKeyedServiceFactory(const char* name,
                                  BrowserStateDependencyManager* manager);
  ~BrowserStateKeyedServiceFactory() override;
        
  KeyedService* GetServiceForBrowserState(web::BrowserState* context,
                                          bool create);
        
  virtual web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const;
        
  virtual bool ServiceIsCreatedWithBrowserState() const;
  bool ServiceIsNULLWhileTesting() const override;

  // Interface for people building a type of BrowserStateKeyedFactory: -------

  // All subclasses of BrowserStateKeyedServiceFactory must return a
  // KeyedService instead of just a BrowserStateKeyedBase.
  virtual std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const = 0;
        
  virtual void BrowserStateShutdown(web::BrowserState* context);
  virtual void BrowserStateDestroyed(web::BrowserState* context);

 private:
  virtual void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) {}

  // KeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      void* context) const final;
  bool IsOffTheRecord(void* context) const final;

  // KeyedServiceBaseFactory:
  void* GetContextToUse(void* context) const final;
  bool ServiceIsCreatedWithContext() const final;
  void ContextShutdown(void* context) final;
  void ContextDestroyed(void* context) final;
  void RegisterPrefs(user_prefs::PrefRegistrySyncable* registry) final;
  void CreateServiceNow(void* context) final;

  DISALLOW_COPY_AND_ASSIGN(BrowserStateKeyedServiceFactory);
};
}

#endif  // browser_state_keyed_service_factory_h
