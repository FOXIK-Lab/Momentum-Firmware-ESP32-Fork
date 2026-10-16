#include <assert.h>
#include <string.h>
#include "../applications/main/wlan_app/webfs_security.h"

int main(void) {
    const char* bad[] = {"/ext/", "/ext//", "/ext/.", "/ext/./", "/ext/..", "/ext/a/../b",
        "/ext/a//b", "/ext/a/", "/ext/a\\..\\b", "/ext/a\nb", "/ext/a\rb",
        "/ext/a\"b", "/ext/a:b", "/external/a", "/int/a", "ext/a"};
    for(size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        assert(!webfs_path_allowed(bad[i], true));
        assert(!webfs_path_allowed(bad[i], false));
    }
    assert(webfs_path_allowed("/ext", true));
    assert(!webfs_path_allowed("/ext", false));
    assert(webfs_path_allowed("/ext/apps/Games/test.fap", false));
    assert(webfs_path_allowed("/ext/.momentum_settings", false));
    assert(webfs_path_allowed("/ext/notes..txt", false));
    char out[256];
    assert(webfs_decode_path("%2Fext%2Fhello%20world.txt", out, sizeof(out)));
    assert(!strcmp(out, "/ext/hello world.txt"));
    assert(!webfs_decode_path("/ext/%00ignored", out, sizeof(out)));
    assert(!webfs_decode_path("/ext/%0Aevil", out, sizeof(out)));
    assert(!webfs_decode_path("/ext/%", out, sizeof(out)));
    assert(!webfs_decode_path("/ext/%a", out, sizeof(out)));
    assert(!webfs_decode_path("/ext/%GG", out, sizeof(out)));
    assert(!webfs_decode_path("/ext/long-file", out, 8));
    assert(webfs_decode_path("%2Fext%2F%2E", out, sizeof(out)));
    assert(!webfs_path_allowed(out, false));
    assert(webfs_decode_path("%2Fext%2F%5C..%5C", out, sizeof(out)));
    assert(!webfs_path_allowed(out, false));
    return 0;
}
