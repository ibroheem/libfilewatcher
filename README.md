# libfilewatcher

## Lightweight C++ Class For File Watcher

Modified For MinGW GCC, tested with v7.2

## Why?
- Modern C++ Style
- No Thread

## TODO
- [ ] Port to other version visual studio
- [ ] Unit test
- [ ] Build to DLL library

## Useage
```
int wmain() {
	using namespace filewatcher;
	using namespace std::chrono;
	using namespace std::tr2::sys;

	filewatcher_t watcher;

	watcher.add_watch(L"C:\\",
		notify_filter::FILE_ALL,
		true,
		[](path const &path, file_action action) {
		std::wcout << path.filename() << "\r\n";
	});

	while (true) {
		watcher.wait_and_update(milliseconds(500));
	}
}
```

