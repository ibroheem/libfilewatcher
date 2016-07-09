#include <iostream>
#include <iomanip>
#include <sstream>
#include <filewatcher.h>

std::string byte_format(uintmax_t bytes, int precision) {
    static std::string size_unit_formatters[] = {
        " B",
        " KB",
        " MB",
        " GB",
        " TB",
        " PB",
        " EB",
        " ZB"
    };

    auto double_bytes = static_cast<double>(bytes);
    auto conversions = 0;
    while (double_bytes > 1000) {
        double_bytes /= 1024;
        conversions++;
    }

    std::string units;
    if ((0 <= conversions) && (conversions <= _countof(size_unit_formatters))) {
        units = size_unit_formatters[conversions];
    }

    std::ostringstream ostr;
    ostr.setf(std::ios::fixed);
    ostr.precision(precision);
    ostr << double_bytes << units;
    return ostr.str();
}

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
		notify_filter::FILE_LAST_WRITE,
		true,
		[](path const &path, file_action action) {
        if (!is_regular_file(path))
            return;
        std::wcout << "\t [" << action << "] " << path.filename() << " -- "
            << byte_format(file_size(path), 2)
            << "\r\n";
	});

	while (true) {
		watcher.wait_and_update(milliseconds(500));
	}
}