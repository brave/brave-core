#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_INITIALIZATION_PROMISE_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_INITIALIZATION_PROMISE_H_

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "partition_alloc/pointers/raw_ref.h"

namespace brave_news {

class BraveNewsPrefManager;
class PublishersController;

class InitializationPromise {
 public:
  enum class State { kNone, kInitializing, kInitialized, kFailed };

  InitializationPromise(size_t max_retries,
                        BraveNewsPrefManager& pref_manager,
                        PublishersController& publishers_controller);
  ~InitializationPromise();

  void OnceInitialized(base::OnceClosure on_initialized);

  bool failed() { return state_ == InitializationPromise::State::kFailed; }
  bool complete() { return on_initializing_prefs_complete_.is_signaled(); }

 private:
  void Initialize();
  void OnGotLocale(const std::string& locale);

  void Notify();

  raw_ref<BraveNewsPrefManager> pref_manager_;
  raw_ref<PublishersController> publishers_controller_;

  base::OneShotEvent on_initializing_prefs_complete_;

  size_t max_retries_ = 0;
  size_t attempts_ = 0;

  State state_ = InitializationPromise::State::kNone;

  base::WeakPtrFactory<InitializationPromise> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_INITIALIZATION_PROMISE_H_
