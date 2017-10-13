#include <iostream>
#include <map>
#include <unordered_set>
#include "./filesystem.hpp"

using namespace std;
using namespace filesystem;

// TAP-compliant test
int failed_nr = 0;
int test_nr = 0;
#define DIAG(out) std::cout << "# " << out << std::endl
#define _OK(cond, has_diag, diag) \
    if (cond) \
        std::cout << "ok " << (++test_nr) << std::endl; \
    else { \
        failed_nr++; \
        std::cout << "not ok " << (++test_nr) << std::endl; \
        DIAG("  Failed test at " << __FILE__ << " line " << __LINE__ << "."); \
        if (has_diag) DIAG(diag); \
    }

#define OK(cond)        _OK((cond),     0, "")
#define NOK(cond)       _OK(!(cond),    0, "")
#define IS(a, b)        _OK((a) == (b), 1, a << "==" << b)
#define ISNT(a, b)      _OK((a) != (b), 1, a << "!=" << b)
#define PASS()          _OK(true,       0, "")
#define FAIL()          _OK(false,      0, "")

#define DONE_TESTING() \
    std::cout << "1.." << test_nr << std::endl; \
    if (failed_nr == 0) { \
        return 0; \
    } else { \
        DIAG("Looks like you failed " << failed_nr << " test" << \
             (failed_nr > 1 ? "s" : "") << " of " << test_nr << "."); \
        return 1; \
    }

// Platform specifics
#ifdef _WIN32
#define VOL     "c:"
#define SEP     "\\"
#else
#define VOL     ""
#define SEP     "/"
#endif

int main(int argc, char **argv) {
    path path1(VOL SEP "dir 1" SEP "dir 2" SEP);
    path path2("dir 3");

    // check empty path
    path p;
    IS(p.length(), 0);
    IS(p, "");

    p = path("");
    IS(p.length(), 0);
    IS(p, "");

#ifdef _WIN32
    IS(path1.length(), 3);
    IS(path1[0], "c:");
    IS(path1[1], "dir 1");
    IS(path1[2], "dir 2");
#else
    IS(path1.length(), 2);
    IS(path1[0], "dir 1");
    IS(path1[1], "dir 2");
#endif

    NOK(path1.exists());

    p = path("a/b/c/d");
    IS(p.slice(1), path("b/c/d"));
    IS(p.slice(2), path("c/d"));
    IS(p.slice(0, 0), path());
    IS(p.slice(0, 1), path("a"));
    IS(p.slice(1, 2), path("b"));
    IS(p.slice(0, 2), path("a/b"));
    IS(p.slice(0, 3), path("a/b/c"));

    IS(path1, VOL SEP "dir 1" SEP "dir 2");

    p = (path1 / path2);
    IS(p, VOL SEP "dir 1" SEP "dir 2" SEP "dir 3");

    p = (path1 / path2).dirname();
    IS(p, VOL SEP "dir 1" SEP "dir 2");

    p = (path1 / path2).dirname().dirname();
    IS(p, VOL SEP "dir 1");

#ifdef _WIN32
    p = (path1 / path2).dirname().dirname().dirname();
    IS(p, VOL);

    p = (path1 / path2).dirname().dirname().dirname().dirname();
    IS(p, "");
#else
    p = (path1 / path2).dirname().dirname().dirname();
    IS(p, SEP);

    p = (path1 / path2).dirname().dirname().dirname().dirname();
    IS(p, SEP);
#endif

    p = path().dirname();
    IS(p, "..");

    cout << (path1/path1.as_relative()) << endl;

    cout << "some/path.ext:operator==() = " << (path("some/path.ext") == path("some/path.ext")) << endl;
    cout << "some/path.ext:operator==() (unequal) = " << (path("some/path.ext") == path("another/path.ext")) << endl;

    map<path, int> pathInts;
    pathInts[path::getcwd()] = 125;
    pathInts[path::getcwd() / "foo"] = 1337;
    cout << "map[cwd] (125) = " << pathInts[path::getcwd()] << endl;
    cout << "map[cwd/foo] (1337) = " << pathInts[path::getcwd() / "foo"] << endl;

    unordered_set<path> pathSet;
    pathSet.insert("foo/bar");
    pathSet.insert("qux/qix");
    pathSet.insert("foo/bar");
    cout << "pathSet size (2) = " << pathSet.size() << endl;

    cout << "nonexistant:exists = " << path("nonexistant").exists() << endl;
    cout << "nonexistant:is_file = " << path("nonexistant").is_file() << endl;
    cout << "nonexistant:is_directory = " << path("nonexistant").is_directory() << endl;
    cout << "nonexistant:basename = " << path("nonexistant").basename() << endl;
    cout << "nonexistant:extension = " << path("nonexistant").extension() << endl;
    cout << "filesystem/path.h:exists = " << path("filesystem/path.h").exists() << endl;
    cout << "filesystem/path.h:is_file = " << path("filesystem/path.h").is_file() << endl;
    cout << "filesystem/path.h:is_directory = " << path("filesystem/path.h").is_directory() << endl;
    cout << "filesystem/path.h:basename = " << path("filesystem/path.h").basename() << endl;
    cout << "filesystem/path.h:extension = " << path("filesystem/path.h").extension() << endl;
    cout << "a/../../foo.c:resolve = " << path("a/../../foo.c").resolve() << endl;
    if (path("filesystem/path.h").exists()) {
        cout << "filesystem/path.h:make_absolute = " << path("filesystem/path.h").make_absolute() << endl;
    }
    cout << "filesystem/path.h:dirname = " << path("filesystem/path.h").dirname() << endl;
    cout << "filesystem/path.h:basename = " << path("filesystem/path.h").basename() << endl;
    cout << "filesystem/path.h:resolve = " << path("filesystem/path.h").resolve() << endl;
    cout << "/a/b/c/../../d/e/foo/../././././bar.h:resolve = " << path("/a/b/c/../../d/e/foo/../././././bar.h").resolve() << endl;
    cout << "filesystem/../path.h:resolve = " << path("filesystem/../path.h").resolve() << endl;
    cout << "filesystem/path.h:resolve /a/b/c = " << path("/a/b/c").resolve("filesystem/path.h") << endl;
    cout << "../filesystem/path.h:resolve /a/b/c = " << path("/a/b/c").resolve("../filesystem/path.h") << endl;
    cout << "../filesystem/path.h:resolve /a/b/../c = " << path("/a/b/../c").resolve("../filesystem/path.h") << endl;
    cout << "../filesystem:exists = " << path("../filesystem").exists() << endl;
    cout << "../filesystem:is_file = " << path("../filesystem").is_file() << endl;
    cout << "../filesystem:is_directory = " << path("../filesystem").is_directory() << endl;
    cout << "../filesystem:extension = " << path("../filesystem").extension() << endl;
    cout << "../filesystem:basename = " << path("../filesystem").basename() << endl;
    cout << "../filesystem/path.h:resolve = " << path("../filesystem/path.h").resolve() << endl;
    if (path("../filesystem").exists()) {
        cout << "../filesystem:make_absolute = " << path("../filesystem").make_absolute() << endl;
    }
    cout << "../../a/b/c/../d:resolve = " << path("../../a/b/c/../d").resolve() << endl;
    cout << "/../../a/b/c/../d:resolve = " << path("/../../a/b/c/../d").resolve() << endl;

    cout << "resolve(filesystem/path.h) = " << resolver().resolve("filesystem/path.h") << endl;
    cout << "resolve(nonexistant) = " << resolver().resolve("nonexistant") << endl;

    // tests for absolute paths that do not exist
    cout << "absolute: ./this_is_not_a_path/file.txt = " << path("./this_is_not_a_path/file.txt").make_absolute() << endl;
    cout << "absolute: this_is_not_a_path/no_leading_dot.tar = " << path("this_is_not_a_path/no_leading_dot.tar").make_absolute() << endl;
    cout << "absolute: . = " << path(".").make_absolute() << endl;
    cout << "absolute: .. = " << path("..").make_absolute() << endl;
    cout << "absolute: /var/tmp = " << path("/var/tmp").make_absolute() << endl;
    cout << "absolute: /var/tmp and .. = " << (path("/var/tmp") / path("..")).make_absolute() << endl;
    cout << "absolute: /not/real = " << path("/not/real").make_absolute() << endl;
    cout << "absoulte: /not/real and .. = " << (path("/not/real") / path("..")).make_absolute() << endl;
    cout << "absolute: /var/tmp/../www/. = " << path("/var/tmp/../www/.").make_absolute() << endl;

    DONE_TESTING();
}
