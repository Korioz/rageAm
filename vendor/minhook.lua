local minhook_dir = "minhook-1.3.3/"

vendor_files (minhook_dir) {
	"MinHook.h"
}

libdirs { minhook_dir }
links { "libMinHook-x64-v141-md" }
