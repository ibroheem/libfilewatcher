//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2015-2016 libfilewatcher project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#include <filewatcher.h>

#ifdef _WIN32
     #include <filewatcher.win32.h>
#else
     static_assert(0, "Not support this platforms");
#endif

namespace filewatcher {

filewatcher_t::filewatcher_t()
	: impl_(std::make_unique<filewatcher_impl_t>(DEFAULT_BUFFER_SIZE)) {
}

filewatcher_t::~filewatcher_t() {
}

void filewatcher_t::add_watch(std::tr2::sys::path const & directory,
	notify_filter filter,
	bool recursive, std::function<void(std::tr2::sys::path const&, file_action)>&& handler) {
	impl_->add_watch(directory, filter, recursive, std::move(handler));
}

void filewatcher_t::remove_watch(std::tr2::sys::path const &directory) {
	impl_->remove_watch(directory);
}

void filewatcher_t::update() {
	impl_->update();
}

void filewatcher_t::wait_and_update(std::chrono::milliseconds timeout) {
	impl_->wait_and_update(timeout);
}

}
