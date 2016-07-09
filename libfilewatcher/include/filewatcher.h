//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2015-2016 libfilewatcher project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <cstdint>
#include <functional>
#include <memory>
#include <chrono>
#include <stdexcept>

#include <experimental/filesystem>

namespace filewatcher {

enum file_action {
	FILE_ADD,
	FILE_DELETE,
	FILE_MODIFIED,
	FILE_RENAMED_OLD_NAME,
	FILE_RENAMED_RENAMED_NEW_NAME
};

enum notify_filter {
	FILE_NAME = 1,
	DIRECTORY_NAME = 2,
	FILE_ATTRIBUTES = 4,
	FILE_SIZE = 8,
	FILE_LAST_ACSESS = 16,
	FILE_LAST_WRITE = 32,
	FILE_CREATION = 64,
	FILE_SECURITY = 128,
	FILE_ALL = 0xFF
};

inline notify_filter operator|(notify_filter a, notify_filter b) {
	return static_cast<notify_filter>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

class not_found_path_exception : public std::exception {
public:
	not_found_path_exception()
		: std::exception("not found path") {
	}

	virtual ~not_found_path_exception() = default;
};

class filewatcher_t {
public:
	filewatcher_t();

	~filewatcher_t();

	filewatcher_t(filewatcher_t const &) = delete;
	filewatcher_t& operator=(filewatcher_t const &) = delete;

	void add_watch(std::tr2::sys::path const &path,
		notify_filter filter,
		bool recursive,
		std::function<void(std::tr2::sys::path const &, file_action)> &&handler);

	void remove_watch(std::tr2::sys::path const &directory);

	void update();

	void wait_and_update(std::chrono::milliseconds timeout);

private:
	class filewatcher_impl_t;
	std::unique_ptr<filewatcher_impl_t> impl_;
};

}
