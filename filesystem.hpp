#ifndef FILESYSTEM_HPP__
#define FILESYSTEM_HPP__
#pragma once
/*
	Filesystem utility library

	Copyright (c) 2015-2017 Wenzel Jakob <wenzel@inf.ethz.ch>
	Copyright (c) 2017 Josh Junon <josh@junon.me>

	All rights reserved. Use of this source code is governed by a
	BSD-style license that can be found in the LICENSE file.
*/

#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <utility>
#include <climits>
#include <cstdio>
// string / wstring conversions
#include <locale>
#include <codecvt>

#if defined(_WIN32)
	#include <windows.h>
#else
	#include <unistd.h>
#endif
#include <sys/stat.h>

#if defined(__linux)
	#include <linux/limits.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// Imported from:                                                            //
// https://github.com/python/cpython/blob/master/Include/osdefs.h            //
/* Operating system dependencies */
#if defined(_WIN32)
	#define FILESYSTEM_SEP L'\\'
	#define FILESYSTEM_ALTSEP L'/'
	#define FILESYSTEM_MAXPATHLEN 256
	#define FILESYSTEM_DELIM L';'
#endif

/* Filename separator */
#if !defined(FILESYSTEM_SEP)
	#define FILESYSTEM_SEP L'/'
#endif

/* Max pathname length */
#ifdef __hpux
	#include <sys/param.h>
	#include <limits.h>
	#if !defined(PATH_MAX)
		#define PATH_MAX MAXPATHLEN
	#endif
#endif

#if !defined(MAXPATHLEN)
	#if defined(PATH_MAX) && PATH_MAX > 1024
		#define MAXPATHLEN PATH_MAX
	#else
		#define MAXPATHLEN 1024
	#endif
#endif

/* Search path entry delimiter */
#if !defined(DELIM)
	#define FILESYSTEM_DELIM L':'
#endif
// end import osdefs.h                                                       //
///////////////////////////////////////////////////////////////////////////////

namespace filesystem {

class path;

inline bool create_directory(const path &);

// WARNING: do *NOT* call these methods directly, these exist as helpers for
//          path::make_absolute.  Direct usage is undefined.
namespace detail {
	// see implementations near bottom of file
	static void joinpath(wchar_t *buffer, wchar_t *stuff);
	static void copy_absolute(wchar_t *path, wchar_t *p, size_t pathlen);
	static void absolutize(wchar_t *path);

	// convert wstr to str, default: std::codecvt_utf8<wchar_t>
	template <typename Char_t = wchar_t, typename Conv_t = std::codecvt_utf8<Char_t>>
	std::string as_string(const std::wstring &wstr) {
		std::wstring_convert<Conv_t, Char_t> conversion;
		return conversion.to_bytes(wstr);
	}

	// convert str to wstr, default: std::codecvt_utf8<wchar_t>
	template <typename Char_t = wchar_t, typename Conv_t = std::codecvt_utf8<Char_t>>
	std::wstring as_wstring(const std::string &str) {
		std::wstring_convert<Conv_t, Char_t> conversion;
		return conversion.from_bytes(str);
	}
}

/**
    \brief Simple class for manipulating paths on Linux/Windows/Mac OS

    This class is just a temporary workaround to avoid the heavy boost
    dependency until boost::filesystem is integrated into the standard template
    library at some point in the future.
*/
class path {
	friend struct std::hash<class filesystem::path>;
public:
	typedef std::string value_type;
	typedef std::vector<value_type> leaf_list;

	enum path_type {
		windows_path = 0,
		posix_path = 1,
#if defined(_WIN32)
		native_path = windows_path
#else
		native_path = posix_path
#endif
	};

	path()
		: type(native_path)
		, absolute(false) {
	}

	path(const path &path)
		: type(path.type)
		, leafs(path.leafs)
		, absolute(path.absolute) {
	}

#if __cplusplus >= 201103L
	path(path &&path)
		: type(path.type)
		, leafs(std::move(path.leafs))
		, absolute(path.absolute) {
	}
#endif

	path(const char *string) {
		this->set(string);
	}

	path(const std::string &string) {
		this->set(string);
	}

#if defined(_WIN32)
	path(const std::wstring &wstring) {
		this->set(wstring);
	}
	path(const wchar_t *wstring) {
		this->set(wstring);
	}
#endif

	path(const leaf_list &leafs)
		: type(native_path)
		, leafs(leafs)
		, absolute(false) {
	}

	size_t length() const {
		return this->leafs.size();
	}

	bool empty() const {
		return this->leafs.empty();
	}

	bool is_absolute() const {
		return this->absolute;
	}

	leaf_list::iterator begin() {
		return this->leafs.begin();
	}

	leaf_list::iterator end() {
		return this->leafs.end();
	}

	const leaf_list::const_iterator cbegin() const {
		return this->leafs.cbegin();
	}

	const leaf_list::const_iterator cend() const {
		return this->leafs.cend();
	}

	path slice(size_t begin) const {
		return path(leaf_list(this->leafs.cbegin() + begin, this->leafs.cend()));
	}

	path slice(size_t begin, size_t end) const {
		return path(leaf_list(this->leafs.cbegin() + begin, this->leafs.cbegin() + end));
	}

	path make_absolute() const {
		try {
			// create a buffer and call the detal absolutize method
			wchar_t buffer[MAXPATHLEN+1];
			std::wstring wstr = detail::as_wstring(this->str());
			wcscpy(buffer, wstr.c_str());
			detail::absolutize(buffer);

			// create the resultant path and return it
			std::string str = detail::as_string(std::wstring(buffer));
			path ret = path(str).resolve(false);// eliminate . or ..
			ret.absolute = true;
			return ret;
		}
		catch(const std::exception &e) {
			throw std::runtime_error(
				std::string("Internal error in filesystem::path::make_absolute: ") + e.what()
			);
		}
	}

	bool exists() const {
#if defined(_WIN32)
		return GetFileAttributesW(wstr().c_str()) != INVALID_FILE_ATTRIBUTES;
#else
		struct stat sb;
		return stat(str().c_str(), &sb) == 0;
#endif
	}

	size_t file_size() const {
#if defined(_WIN32)
		struct _stati64 sb;

		if (_wstati64(wstr().c_str(), &sb) != 0) {
			throw std::runtime_error("path::file_size(): cannot stat file \"" + str() + "\"!");
		}

#else
		struct stat sb;

		if (stat(str().c_str(), &sb) != 0) {
			throw std::runtime_error("path::file_size(): cannot stat file \"" + str() + "\"!");
		}

#endif
		return (size_t) sb.st_size;
	}

	bool is_directory() const {
#if defined(_WIN32)
		DWORD result = GetFileAttributesW(wstr().c_str());

		if (result == INVALID_FILE_ATTRIBUTES) {
			return false;
		}

		return (result & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
		struct stat sb;

		if (stat(str().c_str(), &sb)) {
			return false;
		}

		return S_ISDIR(sb.st_mode);
#endif
	}

	bool is_file() const {
#if defined(_WIN32)
		DWORD attr = GetFileAttributesW(wstr().c_str());
		return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
		struct stat sb;

		if (stat(str().c_str(), &sb)) {
			return false;
		}

		return S_ISREG(sb.st_mode);
#endif
	}

	std::string extension() const {
		const std::string &name = basename();
		size_t pos = name.find_last_of(".");

		if (pos == std::string::npos) {
			return "";
		}

		return name.substr(pos + 1);
	}

	path dirname() const {
		path result;
		result.absolute = this->absolute;

		if (this->leafs.empty()) {
			if (!this->absolute) {
				result.leafs.push_back("..");
			}
		} else {
			size_t until = this->leafs.size() - 1;

			for (size_t i = 0; i < until; ++i) {
				result.leafs.push_back(this->leafs[i]);
			}
		}

		return result;
	}

	std::string operator [](size_t i) const {
		return this->leafs[i];
	}

	path operator /(const path &other) const {
		if (other.absolute) {
			throw std::runtime_error("path::operator/(): expected a relative path!");
		}

		if (this->type != other.type) {
			throw std::runtime_error("path::operator/(): expected a path of the same type!");
		}

		path result(*this);

		for (size_t i = 0; i < other.leafs.size(); ++i) {
			result.leafs.push_back(other.leafs[i]);
		}

		return result;
	}

	std::string str(path_type type = native_path) const {
		std::ostringstream oss;

		if (this->type == posix_path && this->absolute) {
			oss << "/";
		}

		for (size_t i = 0; i < this->leafs.size(); ++i) {
			oss << this->leafs[i];

			if (i + 1 < this->leafs.size()) {
				if (type == posix_path) {
					oss << '/';
				} else {
					oss << '\\';
				}
			}
		}

		return oss.str();
	}

	void set(const std::string &str, path_type type = native_path) {
		this->type = type;

		if (type == windows_path) {
			this->leafs = tokenize(str, "/\\");
			this->absolute = str.size() >= 2 && std::isalpha(str[0]) && str[1] == ':';
		} else {
			this->leafs = tokenize(str, "/");
			this->absolute = !str.empty() && str[0] == '/';
		}
	}

	std::string basename() const {
		if (this->empty()) {
			return "";
		}

		return this->leafs.back();
	}

	path resolve(bool tryabsolute = true) const {
		path result(*this);

		if (tryabsolute && this->exists()) {
			result = this->make_absolute();
		}

		for (auto itr = result.leafs.begin(); itr != result.leafs.end();) {
			if (*itr == ".") {
				itr = result.leafs.erase(itr);
				continue;
			}

			if (*itr == "..") {
				if (itr == result.leafs.begin()) {
					if (result.absolute) {
						itr = result.leafs.erase(itr);
					} else {
						++itr;
					}
				} else if (*(itr - 1) != "..") {
					itr = result.leafs.erase(itr - 1);
					itr = result.leafs.erase(itr);
				} else {
					++itr;
				}

				continue;
			}

			++itr;
		}

		return result;
	}

	path as_relative() const {
		path result(*this);
		result.absolute = false;
		return result;
	}

	path as_absolute() const {
		path result(*this);
		result.absolute = true;
		return result;
	}

	path resolve(const path &to) const {
		path result = this->resolve();

		if (!result.absolute) {
			throw std::runtime_error("path::resolve(): this path must be absolute, must exist so make_absolute() works");
		}

		if (this->type != result.type) {
			throw std::runtime_error("path::resolve(): 'to' path has different type");
		}

		if (to.absolute) {
			return to;
		}

		for (std::string leaf : to.leafs) {
			if (leaf == ".") {
				continue;
			}

			if (leaf == ".." && result.leafs.size()) {
				result.leafs.pop_back();
				continue;
			}

			result.leafs.push_back(leaf);
		}

		return result;
	}

	path &operator =(const path &path) {
		this->type = path.type;
		this->leafs = path.leafs;
		this->absolute = path.absolute;
		return *this;
	}

	path &operator =(path &&path) {
		if (this != &path) {
			this->type = path.type;
			this->leafs = std::move(path.leafs);
			this->absolute = path.absolute;
		}

		return *this;
	}

	friend std::ostream &operator <<(std::ostream &os, const path &path) {
		os << path.str();
		return os;
	}

	bool remove_file() {
#if !defined(_WIN32)
		return std::remove(str().c_str()) == 0;
#else
		return DeleteFileW(wstr().c_str()) != 0;
#endif
	}

	bool resize_file(size_t target_length) {
#if !defined(_WIN32)
		return ::truncate(str().c_str(), (off_t) target_length) == 0;
#else
		HANDLE handle = CreateFileW(wstr().c_str(), GENERIC_WRITE, 0, nullptr, 0, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (handle == INVALID_HANDLE_VALUE) {
			return false;
		}

		LARGE_INTEGER size;
		size.QuadPart = (LONGLONG) target_length;

		if (SetFilePointerEx(handle, size, NULL, FILE_BEGIN) == 0) {
			CloseHandle(handle);
			return false;
		}

		if (SetEndOfFile(handle) == 0) {
			CloseHandle(handle);
			return false;
		}

		CloseHandle(handle);
		return true;
#endif
	}

	static path getcwd() {
#if !defined(_WIN32)
		char temp[PATH_MAX];

		if (::getcwd(temp, PATH_MAX) == NULL) {
			throw std::runtime_error("Internal error in getcwd(): " + std::string(strerror(errno)));
		}

		return path(temp);
#else
		std::wstring temp(MAX_PATH, '\0');

		if (!_wgetcwd(&temp[0], MAX_PATH)) {
			throw std::runtime_error("Internal error in getcwd(): " + std::to_string(GetLastError()));
		}

		return path(temp.c_str());
#endif
	}

#if defined(_WIN32)
	std::wstring wstr(path_type type = native_path) const {
		std::string temp = str(type);
		int size = MultiByteToWideChar(CP_UTF8, 0, &temp[0], (int)temp.size(), NULL, 0);
		std::wstring result(size, 0);
		MultiByteToWideChar(CP_UTF8, 0, &temp[0], (int)temp.size(), &result[0], size);
		return result;
	}


	void set(const std::wstring &wstring, path_type type = native_path) {
		std::string string;

		if (!wstring.empty()) {
			int size = WideCharToMultiByte(CP_UTF8, 0, &wstring[0], (int)wstring.size(), NULL, 0, NULL, NULL);
			string.resize(size, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstring[0], (int)wstring.size(), &string[0], size, NULL, NULL);
		}

		this->set(string, type);
	}

	path &operator =(const std::wstring &str) {
		this->set(str);
		return *this;
	}
#endif

	bool operator ==(const path &left) const {
		return left.leafs == this->leafs;
	}

	bool operator !=(const path &left) const {
		return left.leafs != this->leafs;
	}

	bool operator <(const path &right) const {
		return std::tie(this->type, this->absolute, this->leafs) < std::tie(right.type, right.absolute, right.leafs);
	}

	/*
		not the safest method! use with care!
	*/
	bool mkdirp() const {
		if (!this->absolute) {
			throw std::runtime_error("path must be absolute to mkdirp()");
		}

		if (this->exists()) {
			return true;
		}

		path parent = this->dirname();

		if (!parent.exists()) {
			if (!parent.mkdirp()) {
				return false;
			}
		}

		return create_directory(*this);
	}

	void push_back(std::string leaf) {
		this->leafs.push_back(leaf);
	}

protected:
	static leaf_list tokenize(const std::string &string, const std::string &delim) {
		std::string::size_type lastPos = 0, pos = string.find_first_of(delim, lastPos);
		leaf_list tokens;

		if (string.empty()) {
			return tokens;    // return empty list for empty input
		}

		while (lastPos != std::string::npos) {
			if (pos != lastPos) {
				tokens.push_back(string.substr(lastPos, pos - lastPos));
			}

			lastPos = pos;

			if (lastPos == std::string::npos || lastPos + 1 == string.length()) {
				break;
			}

			pos = string.find_first_of(delim, ++lastPos);
		}

		return tokens;
	}

protected:
	path_type type;
	leaf_list leafs;
	bool absolute;
};

inline bool create_directory(const path &p) {
#if defined(_WIN32)
	return CreateDirectoryW(p.wstr().c_str(), NULL) != 0;
#else
	return mkdir(p.str().c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == 0;
#endif
}

/**
	\brief Simple class for resolving paths on Linux/Windows/Mac OS

	This convenience class looks for a file or directory given its name
	and a set of search paths. The implementation walks through the
	search paths in order and stops once the file is found.
*/
class resolver {
public:
	typedef std::vector<path>::iterator iterator;
	typedef std::vector<path>::const_iterator const_iterator;

	resolver() {
		m_paths.push_back(path::getcwd());
	}

	size_t size() const {
		return m_paths.size();
	}

	iterator begin() {
		return m_paths.begin();
	}
	iterator end()   {
		return m_paths.end();
	}

	const_iterator begin() const {
		return m_paths.begin();
	}
	const_iterator end()   const {
		return m_paths.end();
	}

	void erase(iterator it) {
		m_paths.erase(it);
	}

	void prepend(const path &path) {
		m_paths.insert(m_paths.begin(), path);
	}
	void append(const path &path) {
		m_paths.push_back(path);
	}
	const path &operator[](size_t index) const {
		return m_paths[index];
	}
	path &operator[](size_t index) {
		return m_paths[index];
	}

	path resolve(const path &value) const {
		for (const_iterator it = m_paths.begin(); it != m_paths.end(); ++it) {
			path combined = *it / value;

			if (combined.exists()) {
				return combined;
			}
		}

		return value;
	}

	friend std::ostream &operator<<(std::ostream &os, const resolver &r) {
		os << "resolver[" << std::endl;

		for (size_t i = 0; i < r.m_paths.size(); ++i) {
			os << "  \"" << r.m_paths[i] << "\"";

			if (i + 1 < r.m_paths.size()) {
				os << ",";
			}

			os << std::endl;
		}

		os << "]";
		return os;
	}

private:
	std::vector<path> m_paths;
};

///////////////////////////////////////////////////////////////////////////////
// Imported from:                                                            //
// https://github.com/python/cpython/blob/master/Modules/getpath.c           //
namespace detail {

/* Add a path component, by appending stuff to buffer.
   buffer must have at least MAXPATHLEN + 1 bytes allocated, and contain a
   NUL-terminated string with no more than MAXPATHLEN characters (not counting
   the trailing NUL).  It's a fatal error if it contains a string longer than
   that (callers must be careful!).  If these requirements are met, it's
   guaranteed that buffer will still be a NUL-terminated string with no more
   than MAXPATHLEN characters at exit.  If stuff is too long, only as much of
   stuff as fits will be appended.
*/
static void joinpath(wchar_t *buffer, wchar_t *stuff) {
	size_t n, k;
	if (stuff[0] == FILESYSTEM_SEP)
		n = 0;
	else {
		n = wcslen(buffer);
		if (n > 0 && buffer[n-1] != FILESYSTEM_SEP && n < MAXPATHLEN)
			buffer[n++] = FILESYSTEM_SEP;
	}
	if (n > MAXPATHLEN)
		throw std::runtime_error("buffer overflow in filesystem.hpp's joinpath()");
	k = wcslen(stuff);
	if (n + k > MAXPATHLEN)
		k = MAXPATHLEN - n;
	wcsncpy(buffer+n, stuff, k);
	buffer[n+k] = '\0';
}

/* copy_absolute requires that path be allocated at least
   MAXPATHLEN + 1 bytes and that p be no more than MAXPATHLEN bytes. */
static void copy_absolute(wchar_t *path, wchar_t *p, size_t pathlen) {
	if (p[0] == FILESYSTEM_SEP)
		wcscpy(path, p);
	else {
		::filesystem::path cwd;
		try {
			cwd = path::getcwd();
		}
		catch(...) {
			/* unable to get the current directory */
			wcscpy(path, p);
			return;
		}
		// current working directory was discovered, convert to wide
		std::wstring cwd_wide = as_wstring(cwd.str());
		wcscpy(path, cwd_wide.c_str());

		if (p[0] == '.' && p[1] == FILESYSTEM_SEP)
			p += 2;
		joinpath(path, p);
	}
}

/* absolutize() requires that path be allocated at least MAXPATHLEN+1 bytes. */
static void absolutize(wchar_t *path) {
	wchar_t buffer[MAXPATHLEN+1];

	if (path[0] == FILESYSTEM_SEP)
		return;
	copy_absolute(buffer, path, MAXPATHLEN+1);
	wcscpy(path, buffer);
}

}// namespace detail
// end import getpath.c                                                      //
///////////////////////////////////////////////////////////////////////////////

}// namespace filesystem

namespace std {

template <>
struct hash<filesystem::path> {
	typedef filesystem::path argument_type;
	typedef std::size_t result_type;

	result_type operator()(const filesystem::path &path) const {
		std::size_t seed { 0 };
		hash_combine(seed, (size_t) path.type);
		hash_combine(seed, path.absolute);

		for (const string &s : path.leafs) {
			hash_combine(seed, s);
		}

		return seed;
	}

private:
	template <class T>
	static inline void hash_combine(std::size_t &seed, const T &v) {
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
};

}// namespace std
#endif
