#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "Arduino.h"
#include "Audio.h"
#include "SD_MMC.h"
#include "esp_system.h"

class SdPlayer {
 public:
  // ESPHome's shared microphone/speaker bus uses I2S0. ESP32-audioI2S gets
  // I2S1 and drives the same physical pins only while SD owns the route.
  SdPlayer() : audio_(1) {}

  void begin(uint8_t bclk, uint8_t lrclk, uint8_t dout) {
    Audio::audio_info_callback = [](Audio::msg_t message) {
      if (instance_ != nullptr && message.e == Audio::evt_eof) {
        instance_->eof_requested_ = true;
      }
    };

    pinout_ready_ = audio_.setPinout(bclk, lrclk, dout);
    audio_.setVolume(volume_);
    initialized_ = true;
    instance_ = this;
  }

  void loop() {
    if (!initialized_ || !pinout_ready_) {
      return;
    }

    audio_.loop();

    if (!active_) {
      return;
    }

    if (eof_requested_) {
      eof_requested_ = false;
      if (!start_next_after_eof()) {
        active_ = false;
        failed_event_ = true;
      }
      return;
    }

    if (!paused_ && !started_event_sent_ && audio_.isRunning()) {
      started_event_sent_ = true;
      started_event_ = true;
    }

    // A failed open is reported immediately by connecttoFS(). A decoder or
    // filesystem failure that happens after opening is reported after the
    // startup grace period, not while the decoder is buffering.
    if (!paused_ && !audio_.isRunning() &&
        millis() - start_millis_ > kStartupTimeoutMs) {
      active_ = false;
      remember_failed_track();
       if (!start_from_current(1)) {
        failed_event_ = true;
      }
    }
  }

  bool scan() {
    tracks_.clear();
    failed_tracks_.clear();

    File root = SD_MMC.open("/");
    if (!root || !root.isDirectory()) {
      return false;
    }

    for (File file = root.openNextFile(); file; file = root.openNextFile()) {
      if (!file.isDirectory()) {
        const String path = file.name();
        String lower = path;
        lower.toLowerCase();
        if (lower.endsWith(".mp3")) {
          tracks_.emplace_back(path.c_str());
        }
      }
      file.close();
    }
    root.close();

    std::sort(tracks_.begin(), tracks_.end());
    if (shuffle_ && tracks_.size() > 1) {
      shuffle_tracks();
    }

    if (tracks_.empty()) {
      current_index_ = 0;
    } else if (current_index_ >= tracks_.size()) {
      current_index_ = 0;
    }
    return true;
  }

  bool start() {
    if (!initialized_ || !pinout_ready_) {
      failed_event_ = true;
      return false;
    }
    if (tracks_.empty() && !scan()) {
      failed_event_ = true;
      return false;
    }
    if (tracks_.empty()) {
      failed_event_ = true;
      return false;
    }
    return start_from_current(1);
  }

  void stop() {
    audio_.stopSong();
    active_ = false;
    paused_ = false;
    eof_requested_ = false;
    started_event_sent_ = false;
  }

  bool pause_resume() {
    if (!active_) {
      return false;
    }
    paused_ = audio_.pauseResume();
    return true;
  }

  void set_volume(float normalized) {
    if (normalized < 0.0f) {
      normalized = 0.0f;
    } else if (normalized > 1.0f) {
      normalized = 1.0f;
    }
    volume_ = static_cast<uint8_t>(normalized * 21.0f + 0.5f);
    audio_.setVolume(volume_);
  }

  bool next() {
    if (tracks_.empty()) {
      return false;
    }
    current_index_ = (current_index_ + 1) % tracks_.size();
    return start_from_current(1);
  }

  bool previous() {
    if (tracks_.empty()) {
      return false;
    }
    current_index_ =
        (current_index_ + tracks_.size() - 1) % tracks_.size();
    return start_from_current(-1);
  }

  void set_shuffle(bool enabled) {
    if (shuffle_ == enabled) {
      return;
    }

    const std::string current = current_path();
    shuffle_ = enabled;
    if (shuffle_) {
      shuffle_tracks();
    } else {
      std::sort(tracks_.begin(), tracks_.end());
    }

    current_index_ = 0;
    for (size_t index = 0; index < tracks_.size(); ++index) {
      if (tracks_[index] == current) {
        current_index_ = index;
        break;
      }
    }
  }

  bool shuffle_enabled() const { return shuffle_; }
  bool active() const { return active_; }
  size_t track_count() const { return tracks_.size(); }
  size_t current_index() const { return tracks_.empty() ? 0 : current_index_; }
  bool paused() const { return paused_; }

  bool take_started_event() {
    const bool result = started_event_;
    started_event_ = false;
    return result;
  }

  bool take_failed_event() {
    const bool result = failed_event_;
    failed_event_ = false;
    return result;
  }

  std::string current_path() const {
    if (tracks_.empty() || current_index_ >= tracks_.size()) {
      return {};
    }
    return tracks_[current_index_];
  }

  std::string current_name() const {
    const std::string path = current_path();
    const size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
  }

  std::string playlist() const {
    std::string result;
    for (size_t index = 0; index < tracks_.size(); ++index) {
      if (index != 0) {
        result.push_back('\n');
      }
      result += tracks_[index];
      if (result.size() >= kPlaylistLimit) {
        result.resize(kPlaylistLimit);
        break;
      }
    }
    return result;
  }

  std::string recent_history() const {
    std::string result;
    for (size_t index = 0; index < recent_tracks_.size(); ++index) {
      if (index != 0) {
        result.push_back('\n');
      }
      result += recent_tracks_[index];
    }
    return result;
  }

 private:
  static constexpr uint32_t kStartupTimeoutMs = 10000;
  static constexpr size_t kPlaylistLimit = 4095;
  static constexpr size_t kRecentHistoryLimit = 20;
  static constexpr size_t kFailedTrackLimit = 64;

  bool start_current() {
    const std::string path = current_path();
    if (path.empty()) {
      failed_event_ = true;
      return false;
    }

    failed_event_ = false;
    audio_.stopSong();
    eof_requested_ = false;
    started_event_ = false;
    started_event_sent_ = false;
    paused_ = false;
    start_millis_ = millis();

    if (!audio_.connecttoFS(SD_MMC, path.c_str())) {
      active_ = false;
      failed_event_ = true;
      return false;
    }

    remember_recent_track(path);
    active_ = true;
    return true;
  }

  bool start_next_after_eof() {
    if (tracks_.empty()) {
      return false;
    }
    current_index_ = (current_index_ + 1) % tracks_.size();
    return start_from_current(1);
  }

  bool start_from_current(int direction) {
    if (tracks_.empty()) {
      return false;
    }

    for (size_t checked = 0; checked < tracks_.size(); ++checked) {
      const std::string path = current_path();
      if (!is_failed_track(path) && start_current()) {
        return true;
      }

      remember_failed_track();
      if (direction > 0) {
        current_index_ = (current_index_ + 1) % tracks_.size();
      } else {
        current_index_ =
            (current_index_ + tracks_.size() - 1) % tracks_.size();
      }
    }
    return false;
  }

  bool is_failed_track(const std::string &path) const {
    return std::find(failed_tracks_.begin(), failed_tracks_.end(), path) !=
           failed_tracks_.end();
  }

  void remember_failed_track() {
    const std::string path = current_path();
    if (path.empty() || is_failed_track(path)) {
      return;
    }
    failed_tracks_.push_back(path);
    if (failed_tracks_.size() > kFailedTrackLimit) {
      failed_tracks_.erase(failed_tracks_.begin());
    }
  }

  void remember_recent_track(const std::string &path) {
    recent_tracks_.erase(
        std::remove(recent_tracks_.begin(), recent_tracks_.end(), path),
        recent_tracks_.end());
    recent_tracks_.insert(recent_tracks_.begin(), path);
    if (recent_tracks_.size() > kRecentHistoryLimit) {
      recent_tracks_.pop_back();
    }
  }

  void shuffle_tracks() {
    for (size_t index = tracks_.size(); index > 1; --index) {
      const size_t other = esp_random() % index;
      std::swap(tracks_[index - 1], tracks_[other]);
    }
  }

  inline static SdPlayer *instance_ = nullptr;
  Audio audio_;
  std::vector<std::string> tracks_;
  std::vector<std::string> recent_tracks_;
  std::vector<std::string> failed_tracks_;
  size_t current_index_{0};
  uint32_t start_millis_{0};
  bool initialized_{false};
  bool pinout_ready_{false};
  bool active_{false};
  bool paused_{false};
  uint8_t volume_{21};
  bool shuffle_{false};
  bool eof_requested_{false};
  bool started_event_sent_{false};
  bool started_event_{false};
  bool failed_event_{false};
};
extern SdPlayer *my_sd_player;
