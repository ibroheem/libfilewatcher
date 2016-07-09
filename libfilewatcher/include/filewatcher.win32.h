//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2015-2016 libfilewatcher project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <unordered_map>

#include <filewatcher.h>

namespace filewatcher {

namespace details {
struct watcher_file_entry;
using watcher_file_entry_ptr = std::shared_ptr<details::watcher_file_entry>;
}

enum {
    // The buffer size might be artifically restricted to a size smaller than 64 kB!
    DEFAULT_BUFFER_SIZE = 8 * 1024UL,
    MAX_BUFFER_SIZE = 64 * 1024UL
};

class filewatcher_t::filewatcher_impl_t {
public:
    explicit filewatcher_impl_t(size_t buffer_size);

    void add_watch(std::tr2::sys::path const & path,
        notify_filter filter,
        bool recursive,
        std::function<void(std::tr2::sys::path const &path, file_action)>&& handler);

    void remove_watch(std::wstring const &directory);

    void update();

    void wait_and_update(std::chrono::milliseconds timeout);

private:
    size_t buffer_size_;
    std::unordered_map<std::wstring, details::watcher_file_entry_ptr> entry_map_;
};

}