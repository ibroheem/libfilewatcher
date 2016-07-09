//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2015-2016 libfilewatcher project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cassert>
#include <vector>
#include <unordered_map>

#include <filewatcher.h>

namespace filewatcher {

#pragma region details
namespace details {

struct watcher_file_entry : public OVERLAPPED, public std::enable_shared_from_this<watcher_file_entry> {
	watcher_file_entry(std::wstring const &directory,
		bool recursive,
		uint32_t filter_flags,
		size_t buffer_size);

	~watcher_file_entry();

	void clear();

	bool recursive;
	uint32_t filter_flags;
	HANDLE directory_handle;
	std::vector<uint8_t> buffer;
	std::function<void(std::wstring const &, file_action)> file_changes_handler;
};

using file_entry_ptr = std::shared_ptr<details::watcher_file_entry>;

void CALLBACK file_changes_callback(DWORD error_code, DWORD number_of_bytes_transfered, LPOVERLAPPED overlapped);

void notify_file_changes(file_entry_ptr entry);

bool read_file_changes(watcher_file_entry* entry, bool clear_callback = false) {
	return ::ReadDirectoryChangesW(entry->directory_handle,
		entry->buffer.data(),
		entry->buffer.size(),
		entry->recursive,
		entry->filter_flags,
		nullptr,
		entry,
		clear_callback ? nullptr : details::file_changes_callback) != FALSE;
}

uint32_t make_flags(notify_filter filter) {
	uint32_t flags = 0;
	if (filter & notify_filter::FILE_NAME) {
		flags |= FILE_NOTIFY_CHANGE_FILE_NAME;
	}
	if (filter & notify_filter::DIRECTORY_NAME) {
		flags |= FILE_NOTIFY_CHANGE_DIR_NAME;
	}
	if (filter & notify_filter::FILE_ATTRIBUTES) {
		flags |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
	}
	if (filter & notify_filter::FILE_LAST_ACSESS) {
		flags |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
	}
	if (filter & notify_filter::FILE_LAST_WRITE) {
		flags |= FILE_NOTIFY_CHANGE_LAST_WRITE;
	}
	if (filter & notify_filter::FILE_CREATION) {
		flags |= FILE_NOTIFY_CHANGE_CREATION;
	}
	if (filter & notify_filter::FILE_SIZE) {
		flags |= FILE_NOTIFY_CHANGE_SIZE;
	}
	return flags;
}

file_action make_action(uint32_t act) {
    file_action action;

    switch (act) {
    case FILE_ACTION_RENAMED_NEW_NAME:
        action = file_action::FILE_RENAMED_RENAMED_NEW_NAME;
        break;
    case FILE_ACTION_ADDED:
        action = file_action::FILE_ADD;
        break;
    case FILE_ACTION_RENAMED_OLD_NAME:
        action = file_action::FILE_RENAMED_OLD_NAME;
        break;
    case FILE_ACTION_REMOVED:
        action = file_action::FILE_DELETE;
        break;
    case FILE_ACTION_MODIFIED:
        action = file_action::FILE_MODIFIED;
        break;
    }
    return action;
}

void CALLBACK file_changes_callback(DWORD error_code,
	DWORD number_of_bytes_transfered,
	LPOVERLAPPED overlapped) {
	if (error_code != ERROR_SUCCESS) {
		return;
	}

	if (!number_of_bytes_transfered) {
		return;
	}

	watcher_file_entry *entry = reinterpret_cast<watcher_file_entry*>(overlapped);

	auto shared_entry = entry->shared_from_this();
	if (!shared_entry) {
		return;
	}

	notify_file_changes(shared_entry);
	entry->clear();
	read_file_changes(entry);
}

void notify_file_changes(file_entry_ptr entry) {
	FILE_NOTIFY_INFORMATION const * notify_info = nullptr;
	size_t offset = 0;

	do {
		notify_info = reinterpret_cast<FILE_NOTIFY_INFORMATION const *>(entry->buffer.data() + offset);
		std::wstring fileName(notify_info->FileName,
            notify_info->FileNameLength / sizeof(wchar_t));
		try {
            entry->file_changes_handler(std::tr2::sys::path(fileName),
                make_action(notify_info->Action));
		} catch (...) {
		}

		offset += notify_info->NextEntryOffset;
	} while (notify_info->NextEntryOffset > 0);
}

watcher_file_entry::watcher_file_entry(std::wstring const &directory,
	bool recursive,
	uint32_t filter_flags,
	size_t buffer_size)
	: recursive(recursive)
	, filter_flags(filter_flags) {
	clear();
	buffer.resize(buffer_size);
	directory_handle = ::CreateFileW(directory.c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		nullptr);
	if (directory_handle == INVALID_HANDLE_VALUE) {
		throw not_found_path_exception();
	}
	hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

void watcher_file_entry::clear() {
	// Initial struct OVERLAPPED
	Internal = 0;
	InternalHigh = 0;
	Offset = 0;
	OffsetHigh = 0;
	Pointer = nullptr;
}

watcher_file_entry::~watcher_file_entry() {
	if (directory_handle == INVALID_HANDLE_VALUE) {
		return;
	}
	::CancelIo(directory_handle);
	details::read_file_changes(this, true);
	if (!HasOverlappedIoCompleted(this)) {
		::SleepEx(5, TRUE);
	}
	::CloseHandle(hEvent);
	::CloseHandle(directory_handle);
}

}
#pragma endregion details

#pragma region implementation
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
	std::unordered_map<std::wstring, details::file_entry_ptr> entry_map_;
};

filewatcher_t::filewatcher_impl_t::filewatcher_impl_t(size_t buffer_size) {
	assert(buffer_size % 4 == 0 && "buffer size must 4byte aligned");
	assert(buffer_size <= MAX_BUFFER_SIZE && "buffer size must small than 64KB");
	buffer_size_ = buffer_size;
}

void filewatcher_t::filewatcher_impl_t::add_watch(std::tr2::sys::path const &path,
	notify_filter filter,
	bool recursive,
	std::function<void(std::tr2::sys::path const&, file_action)> &&handler) {
	if (!std::tr2::sys::exists(path)) {
		throw not_found_path_exception();
	}
	auto entry = std::make_shared<details::watcher_file_entry>(path,
		recursive,
		details::make_flags(filter),
		buffer_size_);
	entry->file_changes_handler = handler;
	entry_map_[path] = entry;
	read_file_changes(entry.get());
}

void filewatcher_t::filewatcher_impl_t::remove_watch(std::wstring const &directory) {
	entry_map_.erase(directory);
}

void filewatcher_t::filewatcher_impl_t::update() {
	::MsgWaitForMultipleObjectsEx(0, nullptr, 0, QS_ALLINPUT, MWMO_ALERTABLE);
}

void filewatcher_t::filewatcher_impl_t::wait_and_update(std::chrono::milliseconds timeout) {
	::SleepEx(static_cast<DWORD>(timeout.count()), TRUE);
}
#pragma endregion implementation

filewatcher_t::filewatcher_t(size_t buffer_size)
	: impl_(std::make_unique<filewatcher_impl_t>(buffer_size)) {
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
