#pragma once
#include <array>
#include <string>
#include <utility>
#include "drivers/key_event.h"

namespace adv {
// Session-only paths. Bound retained string capacity as well as entry count.
class PathHistory {
public:
    static constexpr size_t kEntries = 8, kPathBytes = 512, kStorageBytes = 1024;
    size_t size() const { return size_; }
    const std::string& at(size_t i) const { return entries_[i]; }
    size_t storage_bytes() const {
        size_t n = 0;
        for (size_t i = 0; i < size_; ++i) n += entries_[i].capacity() + 1;
        return n;
    }
    void remember(const std::string& value) {
        if (value.empty() || value.size() > kPathBytes || (size_ && entries_[0] == value)) return;
        std::string copy(value);
        if (copy.capacity() + 1 > kStorageBytes) return;
        while (size_ && (size_ == kEntries || storage_bytes() + copy.capacity() + 1 > kStorageBytes)) {
            std::string().swap(entries_[--size_]);
        }
        for (size_t i = size_; i > 0; --i) entries_[i] = std::move(entries_[i - 1]);
        entries_[0] = std::move(copy);
        ++size_;
    }
private:
    std::array<std::string, kEntries> entries_;
    size_t size_ = 0;
};

class TextInputModel {
public:
    enum class Result { kEditing, kSubmit, kCancel };
    TextInputModel(const std::string& initial, bool secret, PathHistory* history)
        : history_(secret ? nullptr : history), value_(initial) {
        if (history_ && history_->size()) { value_ = history_->at(0); index_ = 0; }
        draft_ = value_;
    }
    const std::string& value() const { return value_; }
    std::string take_value() { return std::move(value_); }
    bool has_history() const { return history_ != nullptr; }
    Result handle(const KeyEvent& event) {
        if (event.kind == KeyKind::kEscape || (event.kind == KeyKind::kCharacter && event.character == '\x1b'))
            return Result::kCancel;
        if (event.kind == KeyKind::kEnter) {
            if (history_) history_->remember(value_);
            return Result::kSubmit;
        }
        if (history_ && event.kind == KeyKind::kCharacter && event.character == '\x10') {
            if (history_->size()) {
                if (index_ < 0) { draft_ = value_; index_ = 0; }
                else if (static_cast<size_t>(index_ + 1) < history_->size()) ++index_;
                value_ = history_->at(index_);
            }
        } else if (history_ && event.kind == KeyKind::kCharacter && event.character == '\x0e') {
            if (index_ > 0) value_ = history_->at(--index_);
            else if (index_ == 0) { index_ = -1; value_ = draft_; }
        } else if (event.kind == KeyKind::kBackspace || event.kind == KeyKind::kDelete) {
            if (!value_.empty()) { value_.pop_back(); index_ = -1; }
        } else if (event.kind == KeyKind::kCharacter && event.character >= 32 && event.character <= 126) {
            if (value_.size() < (history_ ? PathHistory::kPathBytes : 96)) {
                value_.push_back(event.character); index_ = -1;
            }
        }
        return Result::kEditing;
    }
private:
    PathHistory* history_;
    std::string value_, draft_;
    int index_ = -1;
};
}
