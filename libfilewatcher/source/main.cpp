#include <iostream>
#include <filewatcher.h>

std::wostream& operator<<(std::wostream &ostr, filewatcher::file_action action) {
	switch (action) {
	case filewatcher::file_action::FILE_ADD:
	     ostr << "File add";
	     break;
	case filewatcher::file_action::FILE_DELETE:
	     ostr << "File delete";
	     break;
	case filewatcher::file_action::FILE_MODIFIED:
	     ostr << "File modified";
	     break;
	case filewatcher::file_action::FILE_RENAMED_OLD_NAME:
	     ostr << "Renamed old name";
	     break;
	case filewatcher::file_action::FILE_RENAMED_RENAMED_NEW_NAME:
	     ostr << "Renamed new name";
	     break;
	}
	return ostr;
}

int wmain() {
	using namespace filewatcher;
	using namespace std::chrono;
	using namespace std::tr2::sys;

	filewatcher_t watcher;

	watcher.add_watch(L"C:\\",
		notify_filter::FILE_ALL,
		true,
		[](path const &path, file_action action) {
		std::wcout << "\t [" << action << "] " << path.filename() << "\r\n";
	});

	while (true) {
		watcher.wait_and_update(milliseconds(500));
	}
}