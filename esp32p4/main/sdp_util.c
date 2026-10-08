#include <string.h>
#include "sdp_util.h"

#define MAX_CAND_LEN 512

int sdp_foreach_candidate(const char *sdp, void (*cb)(const char *cand, void *ctx), void *ctx)
{
    if (sdp == NULL) {
        return 0;
    }
    int count = 0;
    const char *line = sdp;
    while (*line) {
        const char *end = line;
        while (*end && *end != '\r' && *end != '\n') {
            end++;
        }
        size_t n = (size_t)(end - line);
        if (n > 2 && strncmp(line, "a=candidate:", 12) == 0 && n - 2 < MAX_CAND_LEN) {
            char cand[MAX_CAND_LEN];
            memcpy(cand, line + 2, n - 2);
            cand[n - 2] = '\0';
            if (cb) {
                cb(cand, ctx);
            }
            count++;
        }
        line = end;
        while (*line == '\r' || *line == '\n') {
            line++;
        }
    }
    return count;
}

int sdp_has_video(const char *sdp)
{
    if (sdp == NULL) {
        return 0;
    }
    if (strncmp(sdp, "m=video", 7) == 0) {
        return 1;
    }
    return strstr(sdp, "\nm=video") != NULL;
}
