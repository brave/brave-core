/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_DELETE_SOON_HELPER_H_
#define BRAVE_BASE_DELETE_SOON_HELPER_H_

namespace base {
// Helper that forces object to invalidate things before object destroyed
// Subclass must implement DeleteSoonImpl and call DeleteSoon before destructor
// of the object. Example usage would be like a object is a WeakPtr which is
// called and destructed on different threads, in this case we can
// DeleteSoonHelper to make sure the WeakPtr is invalidate on certain thread.
// See tor::TorFileWatcher as a reference.
template <typename T>
class DeleteSoonHelper {
 public:
  constexpr DeleteSoonHelper() = default;
  explicit DeleteSoonHelper(std::nullptr_t) = delete;

  DeleteSoonHelper(const DeleteSoonHelper&) = delete;
  DeleteSoonHelper& operator=(const DeleteSoonHelper&) = delete;

  DeleteSoonHelper(DeleteSoonHelper&&) noexcept = default;
  DeleteSoonHelper& operator=(DeleteSoonHelper&&) noexcept = default;

  ~DeleteSoonHelper() { DCHECK(delete_soon_called); }

  void DeleteSoon() const& {
    static_assert(
        !sizeof(*this),
        "DeleteSoonHelper::DeleteSoon() may only be invoked on a non-const "
        "rvalue, i.e. std::move(*obj.release()).DeleteSoon().");
    NOTREACHED();
  }

  void DeleteSoon() && {
    delete_soon_called = true;
    DeleteSoonImpl();
  }

 protected:
  virtual void DeleteSoonImpl() = 0;

 private:
  bool delete_soon_called = false;
};

}  // namespace base

#endif  // BRAVE_BASE_DELETE_SOON_HELPER_H_
