// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UI_BRAVE_ADS_PUBLIC_CPP_NOTIFICATION_H_
#define BRAVE_UI_BRAVE_ADS_PUBLIC_CPP_NOTIFICATION_H_

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/ui/brave_ads/public/cpp/brave_ads_public_export.h"
#include "brave/ui/brave_ads/public/cpp/notification_delegate.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/vector_icon_types.h"
#include "url/gurl.h"

namespace gfx {
struct VectorIcon;
}  // namespace gfx

namespace brave_ads {

// Represents a button to be shown as part of a notification.
struct BRAVE_ADS_PUBLIC_EXPORT ButtonInfo {
  explicit ButtonInfo(const std::u16string& title);
  ButtonInfo(const ButtonInfo& other);
  ButtonInfo();
  ~ButtonInfo();
  ButtonInfo& operator=(const ButtonInfo& other);

  // Title that should be displayed on the notification button.
  std::u16string title;

  // Icon that should be displayed on the notification button. Optional. On some
  // platforms, a mask will be applied to the icon, to match the visual
  // requirements of the notification. As with Android, MD notifications don't
  // display this icon.
  gfx::Image icon;

  // The placeholder string that should be displayed in the input field for
  // text input type buttons until the user has entered a response themselves.
  // If the value is null, there is no input field associated with the button.
  base::Optional<std::u16string> placeholder;
};

// Represents rich features available for notifications.
class BRAVE_ADS_PUBLIC_EXPORT RichNotificationData {
 public:
  RichNotificationData();
  RichNotificationData(const RichNotificationData& other);
  ~RichNotificationData();

  // Whether the notification should remain on screen indefinitely.
  bool never_timeout = false;

  // Time indicating when the notification was shown. Defaults to the time at
  // which the RichNotificationData instance is constructed.
  base::Time timestamp;

  // Context message to display below the notification's content. Optional. May
  // not be used for notifications that have an explicit origin URL set.
  std::u16string context_message;

  // Large image to display on the notification. Optional.
  gfx::Image image;

  // Small badge to display on the notification to illustrate the source of the
  // notification. Optional.
  gfx::Image small_image;

  // Vector version of |small_image|.
  // Used by Notification::GenerateMaskedSmallIcon.
  // If not available, |small_image| will be used by the method. Optional.
  //
  // Due to the restriction of CreateVectorIcon, this should be a pointer to
  // globally defined VectorIcon instance e.g. kNotificationCapsLockIcon.
  // gfx::Image created by gfx::CreateVectorIcon internally stores reference to
  // VectorIcon, so the VectorIcon should live longer than gfx::Image instance.
  // As a temporary solution to this problem, we make this variable a pointer
  // and only pass globally defined constants.
  // TODO(tetsui): Remove the pointer, after fixing VectorIconSource not to
  // retain VectorIcon reference.  https://crbug.com/760866
  const gfx::VectorIcon* vector_small_image = &gfx::kNoneIcon;

  // Buttons that should show up on the notification. A maximum of 16 buttons
  // is supported by the current implementation, but this may differ between
  // platforms.
  std::vector<ButtonInfo> buttons;

  // Whether updates to the visible notification should be announced to users
  // depending on visual assistance systems.
  bool should_make_spoken_feedback_for_popup_updates = true;

#if defined(OS_CHROMEOS)
  // Flag if the notification is pinned. If true, the notification is pinned
  // and the user can't remove it.
  bool pinned = false;
#endif  // defined(OS_CHROMEOS)

  // Vibration pattern to play when displaying the notification. There must be
  // an odd number of entries in this pattern when it's set: numbers of
  // milliseconds to vibrate separated by numbers of milliseconds to pause.
  std::vector<int> vibration_pattern;

  // Whether the vibration pattern and other applicable announcement mechanisms
  // should be considered when updating the notification.
  bool renotify = false;

  // Whether all announcement mechansims should be suppressed when displaying
  // the notification.
  bool silent = false;

  // Unified theme color used in new style notification.
  // Usually, it should not be set directly.
  // For system notification, ash::CreateSystemNotification with
  // SystemNotificationWarningLevel should be used.
  SkColor accent_color = SK_ColorTRANSPARENT;

  // Controls whether a snooze button should appear on the notification.
  bool should_show_snooze_button = false;
};

class BRAVE_ADS_PUBLIC_EXPORT Notification {
 public:
  // Creates a new notification.
  //
  // |type|: Type of the notification that dictates the layout.
  // |id|: Identifier of the notification. Showing a notification that shares
  //       its profile and identifier with an already visible notification will
  //       replace the former one
  // |title|: Title of the notification.
  // |message|: Body text of the notification. May not be used for certain
  //            values of |type|, for example list-style notifications.
  // |display_source|: Textual representation of who's shown the notification.
  // |origin_url|: URL of the website responsible for showing the notification.
  // |optional_fields|: Rich data that can be used to assign more elaborate
  //                    features to notifications.
  // |delegate|: Delegate that will influence the behaviour of this notification
  //             and receives events on its behalf. May be omitted.
  Notification(const std::string& id,
               const std::u16string& title,
               const std::u16string& message,
               const std::u16string& display_source,
               const GURL& origin_url,
               const RichNotificationData& optional_fields,
               scoped_refptr<NotificationDelegate> delegate);

  // Creates a copy of the |other| notification. The delegate, if any, will be
  // identical for both the Notification instances. The |id| of the notification
  // will be replaced by the given value.
  Notification(const std::string& id, const Notification& other);

  // Creates a copy of the |other| notification. The delegate will be replaced
  // by |delegate|.
  Notification(scoped_refptr<NotificationDelegate> delegate,
               const Notification& other);

  // Creates a copy of the |other| notification. The delegate, if any, will be
  // identical for both the Notification instances.
  Notification(const Notification& other);

  Notification& operator=(const Notification& other);

  virtual ~Notification();

  // Performs a deep copy of |notification|, including images and (optionally)
  // the body image, small image, and icon images which are not supported on all
  // platforms.
  static std::unique_ptr<Notification> DeepCopy(
      const Notification& notification,
      bool include_body_image,
      bool include_small_image,
      bool include_icon_images);

  // Uniquely identifies a notification in the message center. For
  // notification front ends that support multiple profiles, this id should
  // identify a unique profile + frontend_notification_id combination. You can
  // Use this id against the MessageCenter interface but not the
  // NotificationUIManager interface.
  const std::string& id() const { return id_; }

  const std::u16string& title() const { return title_; }
  void set_title(const std::u16string& title) { title_ = title; }

  const std::u16string& message() const { return message_; }
  void set_message(const std::u16string& message) { message_ = message; }

  // The origin URL of the script which requested the notification.
  // Can be empty if the notification is requested by an extension or
  // Chrome app.
  const GURL& origin_url() const { return origin_url_; }
  void set_origin_url(const GURL& origin_url) { origin_url_ = origin_url; }

  // A display string for the source of the notification.
  const std::u16string& display_source() const { return display_source_; }

  void set_profile_id(const std::string& profile_id) {}

  // This vibration_pattern property currently has no effect on
  // non-Android platforms.
  const std::vector<int>& vibration_pattern() const {
    return optional_fields_.vibration_pattern;
  }
  void set_vibration_pattern(const std::vector<int>& vibration_pattern) {
    optional_fields_.vibration_pattern = vibration_pattern;
  }

  // This property currently only works in platforms that support native
  // notifications.
  // It determines whether the sound and vibration effects should signal
  // if the notification is replacing another notification.
  bool renotify() const { return optional_fields_.renotify; }
  void set_renotify(bool renotify) { optional_fields_.renotify = renotify; }

  // This property currently has no effect on non-Android platforms.
  bool silent() const { return optional_fields_.silent; }
  void set_silent(bool silent) { optional_fields_.silent = silent; }

  base::Time timestamp() const { return optional_fields_.timestamp; }
  void set_timestamp(const base::Time& timestamp) {
    optional_fields_.timestamp = timestamp;
  }

  const std::u16string context_message() const {
    return optional_fields_.context_message;
  }

  void set_context_message(const std::u16string& context_message) {
    optional_fields_.context_message = context_message;
  }

  // Decides if the notification origin should be used as a context message
  bool UseOriginAsContextMessage() const;

  // Images fetched asynchronously.
  const gfx::Image& icon() const { return icon_; }
  void set_icon(const gfx::Image& icon) { icon_ = icon; }

  const gfx::Image& image() const { return optional_fields_.image; }
  void set_image(const gfx::Image& image) { optional_fields_.image = image; }

  const gfx::Image& small_image() const { return optional_fields_.small_image; }
  void set_small_image(const gfx::Image& image) {
    optional_fields_.small_image = image;
  }

  const gfx::VectorIcon& vector_small_image() const {
    return *optional_fields_.vector_small_image;
  }
  // Due to the restriction of CreateVectorIcon, this should be a pointer to
  // globally defined VectorIcon instance e.g. kNotificationCapsLockIcon.
  // See detailed comment in RichNotificationData::vector_small_image.
  void set_vector_small_image(const gfx::VectorIcon& image) {
    optional_fields_.vector_small_image = &image;
  }

  // Mask the color of |small_image| to the given |color|.
  // If |vector_small_image| is available, it returns the vector image
  // filled by the |color|.
  // Otherwise, it uses alpha channel of the rasterized |small_image| for
  // masking.
  gfx::Image GenerateMaskedSmallIcon(int dip_size, SkColor color) const;

  // Buttons, with icons fetched asynchronously.
  const std::vector<ButtonInfo>& buttons() const {
    return optional_fields_.buttons;
  }
  void set_buttons(const std::vector<ButtonInfo>& buttons) {
    optional_fields_.buttons = buttons;
  }
  void SetButtonIcon(size_t index, const gfx::Image& icon);

  // Used to keep the order of notifications with the same timestamp.
  // The notification with lesser serial_number is considered 'older'.
  unsigned serial_number() { return serial_number_; }

  // Gets and sets whether the notifiction should remain onscreen permanently.
  bool never_timeout() const { return optional_fields_.never_timeout; }
  void set_never_timeout(bool never_timeout) {
    optional_fields_.never_timeout = never_timeout;
  }

  bool pinned() const {
#if defined(OS_CHROMEOS)
    return optional_fields_.pinned;
#else
    return false;
#endif  // defined(OS_CHROMEOS)
  }
#if defined(OS_CHROMEOS)
  void set_pinned(bool pinned) { optional_fields_.pinned = pinned; }
#endif  // defined(OS_CHROMEOS)

  SkColor accent_color() const { return optional_fields_.accent_color; }
  void set_accent_color(SkColor accent_color) {
    optional_fields_.accent_color = accent_color;
  }

  bool should_show_snooze_button() const {
    return optional_fields_.should_show_snooze_button;
  }

  NotificationDelegate* delegate() const { return delegate_.get(); }

  const RichNotificationData& rich_notification_data() const {
    return optional_fields_;
  }

  void set_delegate(scoped_refptr<NotificationDelegate> delegate) {
    DCHECK(!delegate_);
    delegate_ = std::move(delegate);
  }

 protected:
  std::string id_;
  std::u16string title_;
  std::u16string message_;

  // Image data for the associated icon, used by Ash when available.
  gfx::Image icon_;

  // The display string for the source of the notification.  Could be
  // the same as |origin_url_|, or the name of an extension.
  // Expected to be a localized user facing string.
  std::u16string display_source_;

 private:
  // The origin URL of the script which requested the notification.
  // Can be empty if requested through a chrome app or extension or if
  // it's a system notification.
  GURL origin_url_;
  RichNotificationData optional_fields_;

  // TODO(estade): these book-keeping fields should be moved into
  // NotificationList.
  unsigned serial_number_;

  // A proxy object that allows access back to the JavaScript object that
  // represents the notification, for firing events.
  scoped_refptr<NotificationDelegate> delegate_;
};

}  // namespace brave_ads

#endif  // BRAVE_UI_BRAVE_ADS_PUBLIC_CPP_NOTIFICATION_H_
