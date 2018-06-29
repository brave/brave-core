/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_HELPER_PLATFORM_H_
#define BRAVELEDGER_BAT_HELPER_PLATFORM_H_

#if defined CHROMIUM_BUILD
#include "base/task_scheduler/post_task.h"
#include "base/bind.h"
#include "base/callback.h"

#else
#include <functional>
#include <iostream>
#include <cassert>
#endif

namespace braveledger_bat_helper {

  struct FETCH_CALLBACK_EXTRA_DATA_ST;
  struct CLIENT_STATE_ST;
  struct PUBLISHER_STATE_ST;
  struct MEDIA_PUBLISHER_INFO;

  using FetchCallbackSignature = void (bool, const std::string&, const FETCH_CALLBACK_EXTRA_DATA_ST&);
  using ReadStateCallbackSignature = void (bool, const CLIENT_STATE_ST&);
  using ReadPublisherStateCallbackSignature = void (bool, const PUBLISHER_STATE_ST&);
  using SimpleCallbackSignature = void (const std::string&);
  using GetMediaPublisherInfoSignature = void(const uint64_t&, const braveledger_bat_helper::MEDIA_PUBLISHER_INFO&);
  using SaveVisitSignature = void(const std::string&, const uint64_t&);


#if defined CHROMIUM_BUILD
  using  FetchCallback = base::Callback<FetchCallbackSignature>;
  using  ReadStateCallback = base::Callback<ReadStateCallbackSignature>;
  using  ReadPublisherStateCallback = base::Callback<ReadPublisherStateCallbackSignature>;
  using  SimpleCallback = base::Callback<SimpleCallbackSignature>;
  using  GetMediaPublisherInfoCallback = base::Callback<GetMediaPublisherInfoSignature>;
  using  SaveVisitCallback = base::Callback<SaveVisitSignature>;

  //Binds a member function of an instance of the type (Base) which has a signature (Signature) to a callback wrapper.  
  template <typename BaseType, typename Signature, typename... Args >
  decltype(auto) bat_mem_fun_binder(BaseType & instance, Signature BaseType::* ptr_to_mem, Args... args)
  {
    return base::Bind(ptr_to_mem, base::Unretained(&instance), args...);
  }

  template <typename Signature, typename... Args >
  decltype(auto) bat_fun_binder(Signature ptr_to_fun, Args... args)
  {
    return base::Bind(ptr_to_fun, args...);
  }

  //working with Chromium Callback.Run()
  template <typename Runnable, typename... Args>
  decltype(auto) run_runnable(Runnable & runnable, Args... args)
  {
    return runnable.Run(args...);
  }

#define bat_mem_fun_binder1 bat_mem_fun_binder
#define bat_mem_fun_binder2 bat_mem_fun_binder
#define bat_mem_fun_binder3 bat_mem_fun_binder

#define bat_fun_binder1 bat_fun_binder
#define bat_fun_binder2 bat_fun_binder
#define bat_fun_binder3 bat_fun_binder



  template <typename Runnable>
  void PostTask(Runnable runnable)
  {
    scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::CreateSequencedTaskRunnerWithTraits({ base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN });

    task_runner->PostTask(FROM_HERE, runnable);
  }


#else
  using FetchCallback = std::function<braveledger_bat_helper::FetchCallbackSignature>;
  using ReadStateCallback =  std::function<braveledger_bat_helper::ReadStateCallbackSignature>;
  using ReadPublisherStateCallback  = std::function<braveledger_bat_helper::ReadPublisherStateCallbackSignature> ;
  using SimpleCallback =  std::function<SimpleCallbackSignature>;
  using  GetMediaPublisherInfoCallback = std::function<GetMediaPublisherInfoSignature>;
  using  SaveVisitCallback = std::function<SaveVisitSignature>;

  //Binds a member function of an instance of the type (Base) which has a signature (Signature) to a callback wrapper.  
  template <typename BaseType, typename Signature, typename ... Args >
  decltype(auto) bat_mem_fun_binder(BaseType & instance, Signature BaseType::* ptr_to_mem, Args ... args)
  {
    return std::bind(ptr_to_mem, &instance, args...);
  }

  template <typename BaseType, typename Signature >
  decltype(auto) bat_mem_fun_binder1(BaseType & instance, Signature BaseType::* ptr_to_mem)
  {
    using namespace std::placeholders;
    return std::bind(ptr_to_mem, &instance, _1);
  }

  template <typename BaseType, typename Signature >
  decltype(auto) bat_mem_fun_binder2(BaseType & instance, Signature BaseType::* ptr_to_mem)
  {
    using namespace std::placeholders;
    return std::bind(ptr_to_mem, &instance, _1, _2);
  }

  template <typename BaseType, typename Signature >
  decltype(auto) bat_mem_fun_binder3(BaseType & instance, Signature BaseType::* ptr_to_mem)
  {
    using namespace std::placeholders;
    return std::bind(ptr_to_mem, &instance, _1, _2, _3);
  }

  template <typename Signature, typename... Args >
  decltype(auto) bat_fun_binder(Signature ptr_to_fun, Args... args)
  {
    return std::bind(ptr_to_fun, args...);
  }

  template <typename Signature>
  decltype(auto) bat_fun_binder1(Signature ptr_to_fun)
  {
    using namespace std::placeholders;
    return std::bind(ptr_to_fun, _1);
  }

  template <typename Signature>
  decltype(auto) bat_fun_binder2(Signature ptr_to_fun)
  {
    using namespace std::placeholders;
    return std::bind(ptr_to_fun, _1, _2);
  }

  template <typename Signature>
  decltype(auto) bat_fun_binder3(Signature ptr_to_fun)
  {
    using namespace std::placeholders;
    return std::bind(ptr_to_fun, _1, _2, _3);
  }

  //working with C++ function.operator() 
  template <typename Runnable, typename ... Args>
  decltype(auto) run_runnable(Runnable & runnable, Args ... args)
  {
    return runnable(args...);
  }

  template <typename Runnable, typename ... Args >
  void PostTask(Runnable runnable, Args ... args)
  {
    runnable(args...);
  }

  // Chromium debug macros redefined
#define DCHECK assert
#define LOG(LEVEL) std::cerr<< std::endl<< #LEVEL << ": "

#endif

std::string GenerateGUID();

void encodeURIComponent(const std::string & instr, std::string & outstr);

void DecodeURLChars(const std::string& input, std::string& output);

} //namespace braveledger_bat_helper

#endif  //BRAVELEDGER_BAT_HELPER_PLATFORM_H_
