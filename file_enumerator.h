// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_FILES_FILE_ENUMERATOR_H_
#define BASE_FILES_FILE_ENUMERATOR_H_

#include <stddef.h>
#include <stdint.h>

#include <stack>
#include <string>
#include <vector>

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(OS_WIN)
#define FILE_PATH_LITERAL(x) L##x
#define PRFilePath "ls"
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
#define FILE_PATH_LITERAL(x) x
#define PRFilePath "s"
#endif // OS_WIN

namespace base {

// A class for enumerating the files in a provided path. The order of the
// results is not guaranteed.
//
// This is blocking. Do not use on critical threads.
//
// Example:
//
//   base::FileEnumerator enum(my_dir, false, base::FileEnumerator::FILES,
//                             FILE_PATH_LITERAL("*.txt"));
//   for (base::FilePath name = enum.Next(); !name.empty(); name = enum.Next())
//     ...
class FileEnumerator {
public:
  // Note: copy & assign supported.
  class FileInfo {
  public:
    FileInfo();
    ~FileInfo();

    bool IsDirectory() const;

    // The name of the file. This will not include any path information. This
    // is in constrast to the value returned by FileEnumerator.Next() which
    // includes the |root_path| passed into the FileEnumerator constructor.
    std::wstring GetName() const;

    int64_t GetSize() const;

#if defined(OS_WIN)
    // Note that the cAlternateFileName (used to hold the "short" 8.3 name)
    // of the WIN32_FIND_DATA will be empty. Since we don't use short file
    // names, we tell Windows to omit it which speeds up the query slightly.
    const WIN32_FIND_DATA &find_data() const { return find_data_; }
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
    const struct stat &stat() const { return stat_; }
#endif

  private:
    friend class FileEnumerator;

#if defined(OS_WIN)
    WIN32_FIND_DATA find_data_;
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
    struct stat stat_;
    FilePath filename_;
#endif
  };

  enum FileType {
    FILES = 1 << 0,
    DIRECTORIES = 1 << 1,
    INCLUDE_DOT_DOT = 1 << 2,
#if defined(OS_POSIX) || defined(OS_FUCHSIA)
    SHOW_SYM_LINKS = 1 << 4,
#endif
  };

  // Search policy for intermediate folders.
  enum class FolderSearchPolicy {
    // Recursive search will pass through folders whose names match the
    // pattern. Inside each one, all files will be returned. Folders with names
    // that do not match the pattern will be ignored within their interior.
    MATCH_ONLY,
    // Recursive search will pass through every folder and perform pattern
    // matching inside each one.
    ALL,
  };

  // |root_path| is the starting directory to search for. It may or may not end
  // in a slash.
  //
  // If |recursive| is true, this will enumerate all matches in any
  // subdirectories matched as well. It does a breadth-first search, so all
  // files in one directory will be returned before any files in a
  // subdirectory.
  //
  // |file_type|, a bit mask of FileType, specifies whether the enumerator
  // should match files, directories, or both.
  //
  // |pattern| is an optional pattern for which files to match. This
  // works like shell globbing. For example, "*.txt" or "Foo???.doc".
  // However, be careful in specifying patterns that aren't cross platform
  // since the underlying code uses OS-specific matching routines.  In general,
  // Windows matching is less featureful than others, so test there first.
  // If unspecified, this will match all files.
  FileEnumerator(const std::wstring &root_path, bool recursive, int file_type);
  FileEnumerator(const std::wstring &root_path, bool recursive, int file_type,
                 const std::wstring &pattern);
  FileEnumerator(const std::wstring &root_path, bool recursive, int file_type,
                 const std::wstring &pattern,
                 FolderSearchPolicy folder_search_policy);
  ~FileEnumerator();

  // Returns the next file or an empty string if there are no more results.
  //
  // The returned path will incorporate the |root_path| passed in the
  // constructor: "<root_path>/file_name.txt". If the |root_path| is absolute,
  // then so will be the result of Next().
  std::wstring Next();

  // Write the file info into |info|.
  FileInfo GetInfo() const;

  static std::wstring Append(std::wstring input, std::wstring component);

private:
  // Returns true if the given path should be skipped in enumeration.
  bool ShouldSkip(const std::wstring &path);

  bool IsTypeMatched(bool is_dir) const;

  bool IsPatternMatched(const std::wstring &src) const;

#if defined(OS_WIN)
  // True when find_data_ is valid.
  bool has_find_data_ = false;
  WIN32_FIND_DATA find_data_;
  HANDLE find_handle_ = INVALID_HANDLE_VALUE;
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
  // The files in the current directory
  std::vector<FileInfo> directory_entries_;

  // The next entry to use from the directory_entries_ vector
  size_t current_directory_entry_;
#endif
  std::wstring root_path_;
  const bool recursive_;
  const int file_type_;
  std::wstring pattern_;
  std::stack<std::wstring> pending_paths_;
  const FolderSearchPolicy folder_search_policy_;
};

} // namespace base

#endif // BASE_FILES_FILE_ENUMERATOR_H_