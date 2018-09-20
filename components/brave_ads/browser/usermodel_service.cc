#include "usermodel_service.h"
#include "base/task_runner_util.h"
#include "base/bind.h"
#include "base/guid.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/i18n/time_formatting.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "user_model.h"
#include "user_model_callback_handler.h"

std::string LoadFileTaskRunner(
    const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);

  // Make sure the file isn't empty.
  if (!success || data.empty()) {
    LOG(ERROR) << "Failed to read file: " << path.MaybeAsASCII();
    return std::string();
  }
  return data;
}

void brave_ads::UsermodelService::OnModelLoaded(const std::string& data) {
  usermodel_.initializePageClassifier(data);
}

void brave_ads::UsermodelService::OnUserProfileLoaded(const std::string& data) {
  user_profile_ = usermodel::UserProfile::FromJSON(data);
}

// `callback` has a WeakPtr so this won't crash if the file finishes
// writing after RewardsServiceImpl has been destroyed
void PostWriteCallback(
    const base::Callback<void(bool success)>& callback,
    scoped_refptr<base::SequencedTaskRunner> reply_task_runner,
    bool write_success) {
  // We can't run |callback| on the current thread. Bounce back to
  // the |reply_task_runner| which is the correct sequenced thread.
  reply_task_runner->PostTask(FROM_HERE,
                              base::Bind(callback, write_success));
}

void brave_ads::UsermodelService::OnUsermodelStateSaved(bool success) {
  //handler->OnLedgerStateSaved(success ? ledger::Result::LEDGER_OK
  //                                    : ledger::Result::NO_LEDGER_STATE);
}

void brave_ads::UsermodelService::SaveUsermodelState(const std::string& state) {
  base::ImportantFileWriter writer(
      usermodel_state_path_, file_task_runner_);

  writer.RegisterOnNextWriteCallbacks(
    base::Closure(),
    base::Bind(
      &PostWriteCallback,
      base::Bind(&brave_ads::UsermodelService::OnUsermodelStateSaved, AsWeakPtr()),
      base::SequencedTaskRunnerHandle::Get()));

  writer.WriteNow(std::make_unique<std::string>(state));
}

brave_ads::UsermodelService::UsermodelService(Profile* profile) :
    usermodel_state_(new UserModelState(profile->GetPath().AppendASCII("user_profile"))),
    file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
        base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    usermodel_state_path_(profile->GetPath().AppendASCII("user_profile")),
    taxonomy_model_path_(profile->GetPath().AppendASCII("taxonomy_model.json")) {

    // load models
    base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadFileTaskRunner, taxonomy_model_path_),
      base::Bind(&UsermodelService::OnModelLoaded, AsWeakPtr()));


    LOG(INFO) << "FILE: " << usermodel_state_path_;

/*
    std::string res;
    usermodel_state_->Get("long_term_profile", &res);

    LOG(INFO) << "RES: " << res;
*/
    // load profile
    /*
    base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadFileTaskRunner, usermodel_state_path_),
      base::Bind(&UsermodelService::OnUserProfileLoaded, AsWeakPtr()));
      */
}

brave_ads::UsermodelService::~UsermodelService() {
  //file_task_runner_->DeleteSoon(FROM_HERE, publisher_info_backend_.release());
}
