/*******************************************************************************************
*
*   FIT VIEWER
*   A simple .fit decoder/viewer with map and track drawing on a map, made in C99 using raylib v6.0
*
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************/

#define TOOL_NAME               "GIT MONITOR"
#define TOOL_SHORT_NAME         "gitmonitor"
#define TOOL_VERSION            "0.6.0"
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
#include <git2.h>
#include "raylib.h"
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// window size (16:9)
const int screenWidth = 1184; 
const int screenHeight = 672;
const int fntSize = 16;
bool isDarkMode = false;
char tempBuffer[64]= { 0 };
bool btnQuit = false;

Rectangle statusBarRect = { 8,32,340,295};     // top/left
Rectangle listRepoRect = { 354,8.0,822,492};  // top/right
Rectangle infoRepoRect = { 8,333,340,369}; // bottom/left
Rectangle commitsRect = {354,506,822,196};  // bottom/right

#define REPOS_LIST_FILE  "repos.txt"
#define REPOS_STATE_FILE "repos_state.txt"
#define APP_CACHE_NAME  TOOL_SHORT_NAME
#define MAX_PATH_LEN 4096

#define MAX_URL_LEN     256
#define MAX_MSG_LEN     128
#define MAX_AUTHOR_LEN  64
#define HASH_LEN        41
#define FETCH_DEPTH     16

typedef struct {
    Color accentColorText;
    Color accentColor;
    Color fgColor;
    Color bgColor;
    Color borderColor;
    Color gridColor;
    Color gridBgColor;
} colorTheme;

static const colorTheme lightTheme = {
    .accentColorText = (Color){249, 249, 252, 255}, 
    .accentColor = (Color){53, 132, 228, 255}, 
    .fgColor = (Color){55, 65, 70, 255},
    .bgColor = (Color){218, 227, 227, 255},
    .borderColor = (Color){76, 86, 106, 255},
    .gridColor = (Color){180, 185, 185, 255},
    .gridBgColor = (Color){239, 246, 246, 255},
};

static const colorTheme darkTheme = {
    .accentColorText = (Color){249, 249, 252, 255}, 
    .accentColor = (Color){190, 33, 55, 255},
    .fgColor = (Color){216, 222, 233, 255},
    .bgColor = (Color){50, 55, 66, 255},
    .borderColor = (Color){116, 122, 133, 255},
    .gridColor = (Color){86, 96, 116, 255},
    .gridBgColor = (Color){61, 68, 83, 255},
};

colorTheme currentTheme;


typedef struct {
    char short_hash[8];
    char full_hash[HASH_LEN];
    char author[MAX_AUTHOR_LEN];
    char date[20];    
    char message[MAX_MSG_LEN];
} CommitInfo;

typedef struct {
    char url[MAX_URL_LEN];
    char branch[32];
    char last_hash[HASH_LEN];
    char current_hash[HASH_LEN];
    int new_commits_count;
    CommitInfo *new_commits;
    bool is_new_repo;
    bool has_updates;
    bool error;
    char error_msg[128];
    atomic_bool checking;
} RepoStatus;

typedef struct {
    RepoStatus *repos;
    size_t count;
    atomic_bool is_running;
    atomic_int completed_count;
    int total_to_check;
    int target_index;
    char last_check_time[32]; // data ora last check
} AppState;

// -----------------------------------------------------------------------------
// FUNZIONI
//------------------------------------------------------------------------------

void ApplyTheme(colorTheme *currentTheme, bool darkMode) {
    if (darkMode) *currentTheme = darkTheme;
    else *currentTheme = lightTheme;
}

// Restituisce il percorso base (es. /home/utente/.cache/miaapp) creandolo se manca.
// Restituisce NULL in caso di errore.
const char *get_base_cache_dir(void) {
    static char base_dir[MAX_PATH_LEN] = {0};

    // Se già calcolato in precedenza, lo restituisce subito
    if (base_dir[0] != '\0') {
        return base_dir;
    }

    const char *xdg_cache = getenv("XDG_CACHE_HOME");
    int written = 0;

    if (xdg_cache != NULL && xdg_cache[0] != '\0') {
        if (!DirectoryExists(xdg_cache)) {
            MakeDirectory(xdg_cache);
        }
        written = snprintf(base_dir, sizeof(base_dir), "%s/%s", xdg_cache, APP_CACHE_NAME);
    } else {
        const char *home = getenv("HOME");
        if (home == NULL || home[0] == '\0') {
            TraceLog(LOG_ERROR, "CACHE: Variabile HOME non trovata");
            return NULL;
        }

        // Assicura che ~/.cache esista prima di creare ~/.cache/miaapp
        char parent_cache[MAX_PATH_LEN] = {0};
        int p_written = snprintf(parent_cache, sizeof(parent_cache), "%s/.cache", home);
        if (p_written < 0 || (size_t)p_written >= sizeof(parent_cache)) {
            TraceLog(LOG_ERROR, "CACHE: Percorso ~/.cache troppo lungo");
            return NULL;
        }
        if (!DirectoryExists(parent_cache)) {
            MakeDirectory(parent_cache);
        }

        written = snprintf(base_dir, sizeof(base_dir), "%s/%s", parent_cache, APP_CACHE_NAME);
    }

    if (written < 0 || (size_t)written >= sizeof(base_dir)) {
        TraceLog(LOG_ERROR, "CACHE: Percorso base troppo lungo");
        base_dir[0] = '\0';
        return NULL;
    }

    // Crea ~/.cache/miaapp se non esiste (funzione di raylib)
    if (!DirectoryExists(base_dir)) {
        if (MakeDirectory(base_dir) != 0) {
            TraceLog(LOG_ERROR, "CACHE: Impossibile creare la cartella [%s]", base_dir);
            base_dir[0] = '\0';
            return NULL;
        }
        TraceLog(LOG_INFO, "CACHE: Creata directory [%s]", base_dir);
    }

    return base_dir;
}


void open_url_in_browser(const char *url) {
    if (!url || (strncmp(url, "https://", 8) != 0 && strncmp(url, "http://", 7) != 0)) {
        return;
    }
    if (strchr(url, '\'') != NULL || strchr(url, '"') != NULL) {
        return;
    }

    char cmd[MAX_URL_LEN + 64];
    snprintf(cmd, sizeof(cmd), "xdg-open '%s' >/dev/null 2>&1 &", url);
    int _unused = system(cmd);
    (void)_unused;
}

// Genera il percorso completo: <XDG_CACHE_HOME>/miaapp/<hash>.git
bool get_cache_path(const char *url, char *out_path, size_t max_len) {
    if (!url || url[0] == '\0' || !out_path || max_len == 0) {
        return false;
    }

    const char *cache_dir = get_base_cache_dir();
    if (!cache_dir) {
        return false;
    }

    // Algoritmo djb2 a 64-bit
    uint64_t hash = 5381;
    const unsigned char *str = (const unsigned char *)url;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + (uint64_t)c;
    }

    int written = snprintf(out_path, max_len, "%s/%016" PRIx64 ".git", cache_dir, hash);
    return (written >= 0 && (size_t)written < max_len);
}

const char* get_short_repo_name(const char *url) {
    const char *p = strstr(url, "https://git");
    if (p) return p + 11;
    const char *last_slash = strrchr(url, '/');
    return last_slash ? last_slash + 1 : url;
}

const char *truncate_url(const char *url, char *dest, size_t max_len) {
    if (!url || !dest || max_len == 0) return "";

    size_t len = strlen(url);
    if (len <= max_len) {
        strcpy(dest, url);
    } else if (max_len > 3) {
        strncpy(dest, url, max_len - 3);
        dest[max_len - 3] = '\0';
        strcat(dest, "...");
    } else {
        strncpy(dest, url, max_len);
        dest[max_len] = '\0';
    }
    return dest;
}
void save_repos_to_file(const char *filename, RepoStatus *repos, size_t count) {
    FILE *f = fopen(filename, "w");
    if (!f) return;
    fprintf(f, "# Inserisci un repository Git pubblico per riga\n");
    for (size_t i = 0; i < count; i++) {
        fprintf(f, "%s\n", repos[i].url);
    }
    fclose(f);
}

RepoStatus* load_repos_from_file(const char *filename, size_t *out_count) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        f = fopen(filename, "w");
        if (f) {
            fprintf(f, "# Inserisci un repository GitHub pubblico per riga\n");
            fprintf(f, "https://://github.comraysan5/raylib\n");
            fprintf(f, "https://://github.comraysan5/raygui\n");
            fclose(f);
        }
        f = fopen(filename, "r");
        if (!f) return NULL;
    }

    RepoStatus *list = NULL;
    size_t count = 0;
    char line[MAX_URL_LEN];

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '\0' || line[0] == '#') continue;

        RepoStatus *tmp = realloc(list, (count + 1) * sizeof(RepoStatus));
        if (!tmp) break;
        list = tmp;

        memset(&list[count], 0, sizeof(RepoStatus));
        snprintf(list[count].url, MAX_URL_LEN, "%s", line);
        atomic_init(&list[count].checking, false);
        count++;
    }

    fclose(f);
    *out_count = count;
    return list;
}

void load_previous_hash(const char *url, char *out_hash) {
    out_hash[0] = '\0';
    FILE *f = fopen(REPOS_STATE_FILE, "r");
    if (!f) return;

    char file_url[MAX_URL_LEN];
    char file_hash[HASH_LEN];
    while (fscanf(f, "%255s %40s", file_url, file_hash) == 2) {
        if (strcmp(url, file_url) == 0) {
            strncpy(out_hash, file_hash, HASH_LEN);
            break;
        }
    }
    fclose(f);
}

void save_current_state(RepoStatus *repos, size_t count) {
    FILE *f_old = fopen(REPOS_STATE_FILE, "r");
    char saved_urls[256][MAX_URL_LEN];
    char saved_hashes[256][HASH_LEN];
    size_t saved_count = 0;

    if (f_old) {
        while (saved_count < 256 &&
               fscanf(f_old, "%255s %40s", saved_urls[saved_count], saved_hashes[saved_count]) == 2) {
            saved_count++;
        }
        fclose(f_old);
    }

    FILE *f = fopen(REPOS_STATE_FILE, "w");
    if (!f) return;

    for (size_t i = 0; i < count; i++) {
        const char *hash_to_save = repos[i].error ? repos[i].last_hash : repos[i].current_hash;

        if (hash_to_save[0] == '\0') {
            for (size_t s = 0; s < saved_count; s++) {
                if (strcmp(repos[i].url, saved_urls[s]) == 0) {
                    hash_to_save = saved_hashes[s];
                    break;
                }
            }
        }

        if (hash_to_save[0] != '\0') {
            fprintf(f, "%s %s\n", repos[i].url, hash_to_save);
        }
    }
    fclose(f);
}

void sanitize_url(char *url) {
    char *start = url;
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') start++;
    if (start != url) memmove(url, start, strlen(start) + 1);

    size_t len = strlen(url);
    while (len > 0 && (url[len - 1] == ' ' || url[len - 1] == '\t' ||
                       url[len - 1] == '\n' || url[len - 1] == '\r' || url[len - 1] == '/')) {
        url[--len] = '\0';
    }
}

bool add_repository(AppState *state, const char *raw_url, int *selected_repo) {
    char clean_url[MAX_URL_LEN];
    strncpy(clean_url, raw_url, MAX_URL_LEN - 1);
    clean_url[MAX_URL_LEN - 1] = '\0';
    sanitize_url(clean_url);

    if (strlen(clean_url) < 8 || strncmp(clean_url, "http", 4) != 0) return false;

    for (size_t i = 0; i < state->count; i++) {
        if (strcmp(state->repos[i].url, clean_url) == 0) {
            *selected_repo = (int)i;
            return false;
        }
    }

    RepoStatus *tmp = realloc(state->repos, (state->count + 1) * sizeof(RepoStatus));
    if (!tmp) return false;
    state->repos = tmp;

    memset(&state->repos[state->count], 0, sizeof(RepoStatus));
    snprintf(state->repos[state->count].url, MAX_URL_LEN, "%s", clean_url);
    atomic_init(&state->repos[state->count].checking, false);

    *selected_repo = (int)state->count;
    state->count++;

    save_repos_to_file(REPOS_LIST_FILE, state->repos, state->count);
    return true;
}

void remove_repository(AppState *state, int *selected_repo) {
    if (*selected_repo < 0 || (size_t)(*selected_repo) >= state->count) return;

    size_t idx = (size_t)(*selected_repo);

    char cache_path[512];
    get_cache_path(state->repos[idx].url, cache_path, sizeof(cache_path));
    char cmd[600];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", cache_path);
    int _unused = system(cmd);
    (void)_unused;

    free(state->repos[idx].new_commits);

    if (idx < state->count - 1) {
        memmove(&state->repos[idx], &state->repos[idx + 1], (state->count - idx - 1) * sizeof(RepoStatus));
    }
    state->count--;

    if (state->count == 0) {
        free(state->repos);
        state->repos = NULL;
        *selected_repo = -1;
    } else {
        RepoStatus *tmp = realloc(state->repos, state->count * sizeof(RepoStatus));
        if (tmp) state->repos = tmp;
        if ((size_t)(*selected_repo) >= state->count) {
            *selected_repo = (int)state->count - 1;
        }
    }

    save_repos_to_file(REPOS_LIST_FILE, state->repos, state->count);
    save_current_state(state->repos, state->count);
}

// --- LOGICA LIBGIT2 (FAST-CHECK + CACHE LOCALE + FETCH) ---

void collect_commit_details(git_repository *repo, RepoStatus *status, const git_oid *new_oid, const git_oid *old_oid) {
    git_revwalk *walker = NULL;
    if (git_revwalk_new(&walker, repo) < 0) return;

    git_revwalk_sorting(walker, GIT_SORT_TIME);
    git_revwalk_push(walker, new_oid);

    if (old_oid != NULL) {
        git_revwalk_hide(walker, old_oid);
    }

    git_oid oid;
    while (git_revwalk_next(&oid, walker) == 0) {
        git_commit *commit = NULL;
        if (git_commit_lookup(&commit, repo, &oid) == 0) {
            CommitInfo *tmp = realloc(status->new_commits, (status->new_commits_count + 1) * sizeof(CommitInfo));
            if (tmp) {
                status->new_commits = tmp;
                CommitInfo *info = &status->new_commits[status->new_commits_count];

                git_oid_tostr(info->short_hash, sizeof(info->short_hash), &oid);
                git_oid_tostr(info->full_hash, sizeof(info->full_hash), &oid);

                const git_signature *author = git_commit_author(commit);
                strncpy(info->author, author ? author->name : "Sconosciuto", MAX_AUTHOR_LEN - 1);
                info->author[MAX_AUTHOR_LEN - 1] = '\0';

                time_t commit_time = (time_t)git_commit_time(commit);
                struct tm tm_buf;
                if (localtime_r(&commit_time, &tm_buf) != NULL) {
                    strftime(info->date, sizeof(info->date), "%d/%m/%Y %H:%M", &tm_buf);
                } else {
                    strcpy(info->date, "Data N/D");
                }

                const char *summary = git_commit_summary(commit);
                strncpy(info->message, summary ? summary : "", MAX_MSG_LEN - 1);
                info->message[MAX_MSG_LEN - 1] = '\0';

                status->new_commits_count++;
            }
            git_commit_free(commit);
        }
        // mostra quanti commit visualizzare a video : provo a mettere come FETCH_DEPTH
        if (old_oid == NULL && status->new_commits_count >= FETCH_DEPTH) break;
    }

    git_revwalk_free(walker);
}

void check_repository(RepoStatus *status) {

    git_remote *remote = NULL;

    if (git_remote_create_detached(&remote, status->url) < 0) {
        status->error = true;
        const git_error *e = git_error_last();
        snprintf(status->error_msg, sizeof(status->error_msg), "%s", e ? e->message : "URL non valido");
        return;
    }

    if (git_remote_connect(remote, GIT_DIRECTION_FETCH, NULL, NULL, NULL) < 0) {
        status->error = true;
        const git_error *e = git_error_last();
        snprintf(status->error_msg, sizeof(status->error_msg), "%s", e ? e->message : "Errore connessione");
        git_remote_free(remote);
        return;
    }

    const git_remote_head **remote_refs = NULL;
    size_t refs_len = 0;


    if (git_remote_ls(&remote_refs, &refs_len, remote) < 0) {
        status->error = true;
        snprintf(status->error_msg, sizeof(status->error_msg), "Errore lettura refs remoti");
        git_remote_disconnect(remote);
        git_remote_free(remote);
        return;
    }

    git_oid remote_oid;
    bool found = false;
    for (size_t i = 0; i < refs_len; i++) {
        if (strcmp(remote_refs[i]->name, "refs/heads/main") == 0) {
            git_oid_cpy(&remote_oid, &remote_refs[i]->oid);
            strcpy(status->branch, "main");
            found = true;
            break;
        }
    }

    if (!found) {
        for (size_t i = 0; i < refs_len; i++) {
            if (strcmp(remote_refs[i]->name, "refs/heads/master") == 0) {
                git_oid_cpy(&remote_oid, &remote_refs[i]->oid);
                strcpy(status->branch, "master");
                found = true;
                break;
            }
        }
    }

    git_remote_disconnect(remote);
    git_remote_free(remote);

    if (!found) {
        status->error = true;
        snprintf(status->error_msg, sizeof(status->error_msg), "Branch main/master non trovato");
        return;
    }

    git_oid_tostr(status->current_hash, HASH_LEN, &remote_oid);
    char cache_path[512];
    get_cache_path(status->url, cache_path, sizeof(cache_path));

    if (status->last_hash[0] != '\0' && strcmp(status->last_hash, status->current_hash) == 0) {
        status->has_updates = false;
        git_repository *cached_repo = NULL;
        if (git_repository_open_bare(&cached_repo, cache_path) == 0) {
            collect_commit_details(cached_repo, status, &remote_oid, NULL);
            git_repository_free(cached_repo);
            if (status->new_commits_count > 0) {
                return;
            }
        }
    }

    git_repository *repo = NULL;
    git_remote *repo_remote = NULL;

    if (git_repository_open_bare(&repo, cache_path) < 0) {
        if (git_repository_init(&repo, cache_path, true) < 0) {
            status->error = true;
            snprintf(status->error_msg, sizeof(status->error_msg), "Errore init cache");
            return;
        }
        git_remote_create(&repo_remote, repo, "origin", status->url);
        } else {
        git_remote_lookup(&repo_remote, repo, "origin");
        }

    if (!repo_remote) {
        status->error = true;
        snprintf(status->error_msg, sizeof(status->error_msg), "Errore configurazione remote");
        git_repository_free(repo);
        return;
    }

    git_fetch_options fetch_opts;
    git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
    fetch_opts.depth = FETCH_DEPTH;
    char refspec[128];
    snprintf(refspec, sizeof(refspec), "+refs/heads/%s:refs/remotes/origin/%s", status->branch, status->branch);
    char *refspecs[] = { refspec };
    git_strarray refs = { refspecs, 1 };
  
    if (git_remote_fetch(repo_remote, &refs, &fetch_opts, NULL) < 0) {
        status->error = true;
        const git_error *e = git_error_last();
        snprintf(status->error_msg, sizeof(status->error_msg), "%s", e ? e->message : "Errore fetch");
        git_remote_free(repo_remote);
        git_repository_free(repo);
        return;
    }

    if (status->last_hash[0] == '\0') {
        status->is_new_repo = true;
        status->has_updates = true;
        collect_commit_details(repo, status, &remote_oid, NULL);
    } else if (strcmp(status->last_hash, status->current_hash) != 0) {
            status->has_updates = true;
            git_oid old_oid;
            if (git_oid_fromstr(&old_oid, status->last_hash) == 0) {
                collect_commit_details(repo, status, &remote_oid, &old_oid);
            }
    } else {
             status->has_updates = false;
             collect_commit_details(repo, status, &remote_oid, NULL);
        }
    git_remote_free(repo_remote);
    git_repository_free(repo);

}

// --- GESTIONE MULTITHREADING (PTHREAD) ---
void* worker_check_single_repo(void *arg) {
    RepoStatus *repo = (RepoStatus *)arg;
    check_repository(repo);
    atomic_store(&repo->checking, false);
return NULL;
}

void prepare_repo_for_check(RepoStatus *r) {
    free(r->new_commits);
    r->new_commits = NULL;
    r->new_commits_count = 0;
    r->is_new_repo = false;
    r->has_updates = false;
    r->error = false;
    r->error_msg[0] = '\0';
    load_previous_hash(r->url, r->last_hash);
    atomic_store(&r->checking, true);
}

void* coordinator_thread(void *arg) {
    AppState *state = (AppState *)arg;
        if (state->target_index >= 0 && (size_t)state->target_index < state->count) {
            size_t idx = (size_t)state->target_index;
            prepare_repo_for_check(&state->repos[idx]);
            pthread_t t;
            pthread_create(&t, NULL, worker_check_single_repo, &state->repos[idx]);
            pthread_join(t, NULL);
            atomic_fetch_add(&state->completed_count, 1);
        } else {
        pthread_t *threads = malloc(state->count * sizeof(pthread_t));

        for (size_t i = 0; i < state->count; i++) {
            prepare_repo_for_check(&state->repos[i]);
            pthread_create(&threads[i], NULL, worker_check_single_repo, &state->repos[i]);
        }
        for (size_t i = 0; i < state->count; i++) {
            pthread_join(threads[i], NULL);
            atomic_fetch_add(&state->completed_count, 1);
        }
        free(threads);
    }

    save_current_state(state->repos, state->count);

    // Salva data e ora dell'ultimo controllo completato
    time_t now = time(NULL);
    struct tm tm_now;
        if (localtime_r(&now, &tm_now) != NULL) {
            strftime(state->last_check_time, sizeof(state->last_check_time), "%d/%m/%Y %H:%M:%S", &tm_now);
        }

    atomic_store(&state->is_running, false);
    return NULL;
}

void start_check(AppState *state, int target_idx) {
    if (atomic_load(&state->is_running) || state->count == 0) return;
    state->target_index = target_idx;
    state->total_to_check = (target_idx >= 0) ? 1 : (int)state->count;
    atomic_store(&state->is_running, true);
    atomic_store(&state->completed_count, 0);
    pthread_t coord;
    pthread_create(&coord, NULL, coordinator_thread, state);
    pthread_detach(coord);
}


int main(void) {

     // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED |  FLAG_MSAA_4X_HINT); //|  FLAG_MSAA_4X_HINT); // | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "gitmonitor");
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works

    // carica fonts con tutti i caratteri da 32 (spazio) a 255 (caratteri estesi europei)
    int startGlyph = 32;
    int endGlyph = 255;
    int glyphCount = endGlyph - startGlyph + 1;

    // array per i codepoints
    int *codepoints = (int *)malloc(sizeof(int) * glyphCount);
    for (int i = 0; i < glyphCount; i++) codepoints[i] = startGlyph + i;

    Font textFont = LoadFontEx("fonts/PixelOperator.ttf", fntSize,codepoints, glyphCount); // all other text
    Font boldFont = LoadFontEx("fonts/PixelOperator-Bold.ttf", fntSize,codepoints, glyphCount); // all other text
    Font h1font = LoadFontEx("fonts/ManropeV5-ExtraBold.otf", 32, codepoints, glyphCount);
    Font h2font = LoadFontEx("fonts/ManropeV5-SemiBold.otf", 26, codepoints, glyphCount);
    Font h3font = LoadFontEx("fonts/ManropeV5-Regular.otf", 24, codepoints, glyphCount);
    Font monoFont = LoadFontEx("fonts/SF-Mono-Semibold.ttf",16,codepoints,glyphCount);

    //mkdir(CACHE_DIR, 0755);
    //if(!DirectoryExists(CACHE_DIR)) MakeDirectory(CACHE_DIR);

    ApplyTheme(&currentTheme, isDarkMode);

    git_libgit2_init();
    AppState state = { 0 };
    atomic_init(&state.is_running, false);
    atomic_init(&state.completed_count, 0);
    state.repos = load_repos_from_file(REPOS_LIST_FILE, &state.count);

    int selected_repo = (state.count > 0) ? 0 : -1;
    Vector2 repo_scroll = { 0, 0 };
    Vector2 commit_scroll = { 0, 0 };
    //char url_input[MAX_URL_LEN] = { 0 };
    //bool url_edit_mode = false;
    char buf[MAX_URL_LEN + 64];
    char infoText[1024];

    // GuiStyle common settings
    GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 2);
    GuiSetStyle(SCROLLBAR,ARROWS_VISIBLE,0); 
    GuiSetStyle(SCROLLBAR,SCROLL_SLIDER_PADDING,0);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL,1);

    // set FPS e fai riapparire finestra dopo caricamento iniziale
    SetTargetFPS(60);
    ClearWindowState(FLAG_WINDOW_HIDDEN);


while (!WindowShouldClose()) {


//----------------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------------


int winW = GetScreenWidth();
int winH = GetScreenHeight();

bool running = atomic_load(&state.is_running);

    // Ottiene il timestamp di modifica del file REPOS_STATE_FILE tramite raylib
    time_t file_time = (time_t)GetFileModTime(REPOS_STATE_FILE);
    struct tm* tm_info = localtime(&file_time);
    strftime(tempBuffer, sizeof(tempBuffer), "%d/%m/%Y %H:%M:%S", tm_info);


    if (btnQuit || IsKeyPressed(KEY_Q)) break;
    if (IsKeyPressed(KEY_T)) {
            isDarkMode = !isDarkMode;
            // Applica tema colori
            ApplyTheme(&currentTheme, isDarkMode);
        }
    
    // -------------------------------------------------------------------------
    // color GuiStyle widgets
    //--------------------------------------------------------------------------
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(currentTheme.gridBgColor));

    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(currentTheme.gridColor));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL,   ColorToInt(currentTheme.gridBgColor));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,   ColorToInt(currentTheme.fgColor));

    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt(currentTheme.gridColor));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED,   ColorToInt(currentTheme.gridBgColor));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED,   ColorToInt(currentTheme.accentColor));

    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, ColorToInt(currentTheme.gridColor));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED,   ColorToInt(currentTheme.gridBgColor));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED,   ColorToInt(currentTheme.fgColor));

    GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, ColorToInt(currentTheme.gridColor));
    GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED,   ColorToInt(currentTheme.gridBgColor));
    GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED,   ColorToInt(currentTheme.gridColor));

//----------------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------------

BeginDrawing();
    ClearBackground(currentTheme.bgColor);
    
    // close button
    btnQuit = GuiButton((Rectangle){8,8,20,20}, "#128#");
    DrawTextEx(boldFont, TextFormat("%s\tversione %s", TOOL_NAME, TOOL_VERSION), (Vector2){36, 10},(float)boldFont.baseSize,-1,currentTheme.fgColor); 

    if (running) GuiDisable();
        if (GuiButton((Rectangle){ 8, winH-68, 128, 28 }, "#77#Aggiorna tutto")) {
        start_check(&state, -1);
        commit_scroll = (Vector2){ 0, 0 };
    }
    if (selected_repo < 0) GuiDisable();
    
    if (GuiButton((Rectangle){ 274, winH-68, 128, 28 }, "#143#Elimina repo")) {
        remove_repository(&state, &selected_repo);
        commit_scroll = (Vector2){ 0, 0 };
    }

    if (!running) GuiEnable();

//
//  STATUS BAR Messages
//
    // spinner statico in alto a destra

    float angle = (float)GetTime() * 360.0f;
    DrawRing((Vector2){ (float) winW - 24, 24 }, 8, 12, 0.0f,360.0f , 14, currentTheme.gridColor);

    if (running) {
        int done = atomic_load(&state.completed_count);
        snprintf(infoText, sizeof(infoText), "Aggiornamento repository in corso: %d / %d repo...", done, state.total_to_check);
        // Draw spinner while checking...
        DrawRing((Vector2){ (float) winW - 24, 24 }, 8, 12, angle, angle + 90, 14, currentTheme.accentColor);
    } else {
        if (state.last_check_time[0] != '\0') 
            snprintf(infoText,sizeof(infoText),"Aggiornamento dei repository completato. Data ultima verifica: %s", state.last_check_time);
        else 
        snprintf(infoText,sizeof(infoText),"In attesa di aggiornameto  %zu repository (%s). Data ultimo controllo: %s", state.count, REPOS_LIST_FILE, tempBuffer);
    }

    GuiStatusBar((Rectangle){ 8, winH-32, winW-16, 22 }, infoText);

// posizionamento pannelli centrali
    float topOffset = 36.0f; // posizione Y pannelli centrali
    float leftWidth = screenWidth / 3.0f; // divisione  tra pannelli sx e dx

// --- PANNELLO SINISTRO: LISTA REPOSITORY ---
    Rectangle leftPanelBounds = { 8, topOffset, leftWidth, (float)winH - (topOffset * 3) };
    float itemHeight = 28.0f; // altezza singola "voce" della lista repo
    Rectangle leftContentRect = { 0, 0, leftWidth - 16, (float)state.count * itemHeight };
    Rectangle leftView = { 0 };

    GuiScrollPanel(leftPanelBounds, NULL, leftContentRect, &repo_scroll, &leftView);
    BeginScissorMode((int)leftView.x, (int)leftView.y, (int)leftView.width, (int)leftView.height);
    
    // lista dei repository
        for (size_t i = 0; i < state.count; i++) {
            float y = leftView.y + repo_scroll.y + (i * itemHeight);
            if (y + itemHeight < leftView.y || y > leftView.y + leftView.height) continue;
            Rectangle rowRect = { leftView.x, y, leftView.width, itemHeight };
            bool isSelected = ((int)i == selected_repo);
            bool mouseInView = CheckCollisionPointRec(GetMousePosition(), leftView);
            GuiEnable();

            // selezione del repo dalla lista
            if (mouseInView && CheckCollisionPointRec(GetMousePosition(), rowRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                selected_repo = (int)i;
                commit_scroll = (Vector2){ 0, 0 };
            }

            Color badgeColor = LIGHTGRAY;
            Color fg = isSelected ? currentTheme.accentColorText : currentTheme.fgColor;
            Color bg = isSelected ? currentTheme.accentColor : (i % 2) ? Fade(currentTheme.bgColor, 0.50f) : currentTheme.gridBgColor;

            // "selezione" repository
            DrawRectangleRec(rowRect, bg);
            if (running || !mouseInView) GuiDisable();
            
            GuiEnable();
            
            // BADGE
            if (atomic_load(&state.repos[i].checking)) badgeColor = ORANGE;
            else if (state.repos[i].error) badgeColor = RED;
            else if (state.repos[i].is_new_repo) badgeColor = SKYBLUE;
            else if (state.repos[i].has_updates) badgeColor = GOLD;
            else if (state.repos[i].current_hash[0] != '\0') badgeColor = GREEN;

            DrawCircleV((Vector2){(int)rowRect.x + 12, (int)rowRect.y + 14}, 6.0f,badgeColor);
            const char *shortName = truncate_url(state.repos[i].url,buf,58);
            DrawTextEx(textFont, shortName,(Vector2){(int)rowRect.x + 24, (int)rowRect.y + 6},(float)textFont.baseSize,-1, fg);
        
        } // fine del for (operazioni per tutte le voci della lista)
    EndScissorMode();

// --- PANNELLO DESTRO: DETTAGLI E COMMIT ---
    float rightX = leftWidth + 16.0f; // distanza dal pannello di sinistra
    float rightWidth = (float)winW - rightX - 8.0f;
    //Rectangle rightPanelBounds = { rightX, topOffset, rightWidth, (float)winH - (topOffset * 3) };
    if (selected_repo >= 0 && (size_t)selected_repo < state.count) {
        RepoStatus *sel = &state.repos[selected_repo];
        bool isRepoChecking = atomic_load(&sel->checking);
    
    if (GuiButton((Rectangle){ 140, winH-68, 130, 28 }, "#185#Apri nel browser")) open_url_in_browser(sel->url);
    // URL del repository
    const char *shortUrl = truncate_url(sel->url,buf,58);
    DrawTextEx(h1font, shortUrl,(Vector2){(int)rightX, (int)topOffset-4},(float)h1font.baseSize,0, currentTheme.fgColor);


    if (isRepoChecking) {
        DrawTextEx(h2font,"Verifica in corso sul server remoto...",(Vector2){(int)rightX, (int)topOffset + 28},(float)h2font.baseSize, 0, currentTheme.fgColor);
    } else if (sel->error) {
        DrawTextEx(h2font, TextFormat("Errore: %s", sel->error_msg),(Vector2){(int)rightX, (int)topOffset + 28},(float)h2font.baseSize,0, RED);
    } else if (sel->current_hash[0] == '\0') {
        DrawTextEx(h2font,"Premi [Aggiorna tutto] per aggiornare lo stato dei repository.", (Vector2){(int)rightX, (int)topOffset + 28},(float)h2font.baseSize, 0, currentTheme.fgColor);
    } else {
        const char *statusStr = sel->is_new_repo ? TextFormat("Nuovo repository (mostra ultimi %d commit)", FETCH_DEPTH) : (sel->has_updates ? TextFormat("Repository aggiornato (%d nuovi commit presenti)", sel->new_commits_count) : TextFormat("Nessun nuovo commit (mostra ultimi %d commit)", FETCH_DEPTH));
        
        // messaggio no commit recenti
        //Color statusCol = sel->has_updates ? currentTheme.accentColor : currentTheme.fgColor;
        DrawTextEx(h2font, TextFormat("Branch: %s - Hash attuale: %.12s", sel->branch, sel->current_hash),(Vector2){(int)rightX, (int)topOffset + 28},(float)h2font.baseSize, 0, currentTheme.fgColor);

        DrawTextEx(h3font, statusStr, (Vector2){(int)rightX, (int)topOffset + 56},(float)h3font.baseSize,0, GRAY);
    }

    float commitsTop = topOffset + 88.0f; // distanza tra dettagli e lista commit 
    Rectangle commitsBounds = { rightX, commitsTop, rightWidth, (float)winH - commitsTop - 40.0f };
    float commitRowH = 42.0f;
    int displayCount = isRepoChecking ? 0 : sel->new_commits_count;
    Rectangle commitsContent = { 0, 0, rightWidth - 16, (float)displayCount * commitRowH };
    Rectangle commitsView = { 0 };


    GuiScrollPanel(commitsBounds, NULL, commitsContent, &commit_scroll, &commitsView);
    BeginScissorMode((int)commitsView.x, (int)commitsView.y, (int)commitsView.width, (int)commitsView.height);

        if (!isRepoChecking && displayCount == 0 && !sel->error) {
            DrawTextEx(h2font,"Nessun commit caricato.",(Vector2){(int)commitsView.x + 15, (int)commitsView.y + 15}, (float)h2font.baseSize,0, RED);
        }

        for (int c = 0; c < displayCount; c++) {
            float cy = commitsView.y + commit_scroll.y + (c * commitRowH);
            if (cy + commitRowH < commitsView.y || cy > commitsView.y + commitsView.height) continue;
            Rectangle cRect = { commitsView.x, cy, commitsView.width, commitRowH};
            DrawRectangleRec(cRect, (c % 2) ? Fade(currentTheme.bgColor, 0.50f) : currentTheme.gridBgColor);
            float commitBtnW = 24.0f;
            Rectangle commitBtnRect = { cRect.x + cRect.width - commitBtnW - 8, cRect.y + 10, commitBtnW, 24 };
           
            if (CheckCollisionPointRec(GetMousePosition(), commitsView)) {
                if (GuiButton(commitBtnRect, "#185#")) {
                char commitUrl[MAX_URL_LEN + 64];
                char baseUrl[MAX_URL_LEN];
                strncpy(baseUrl, sel->url, MAX_URL_LEN - 1);
                baseUrl[MAX_URL_LEN - 1] = '\0';
                size_t ulen = strlen(baseUrl);
                if (ulen > 4 && strcmp(baseUrl + ulen - 4, ".git") == 0) {
                    baseUrl[ulen - 4] = '\0';
                }
                snprintf(commitUrl, sizeof(commitUrl), "%s/commit/%s", baseUrl, sel->new_commits[c].full_hash);
                open_url_in_browser(commitUrl);
                }
            } else {
                GuiDisable();
                GuiButton(commitBtnRect, "#185#"); // ridisegna bottone apri a fianco del commit
                GuiEnable();
            }
            // Riga 1: Hash, Data/Ora e Autore
            DrawTextEx(monoFont, TextFormat("[%s]  %s  -  %s", sel->new_commits[c].short_hash, sel->new_commits[c].date, sel->new_commits[c].author),(Vector2){(int)cRect.x + 4, (int)cRect.y + 4},(float)monoFont.baseSize,0, currentTheme.fgColor);
            // Riga 2: Messaggio del commit
            const char *shortCommit = truncate_url(sel->new_commits[c].message,buf,108);
            DrawTextEx(textFont, shortCommit ,(Vector2){(int)cRect.x + 4, (int)cRect.y + 22},(float)textFont.baseSize,0, GRAY);
        }
    EndScissorMode();
    } else DrawTextEx(h2font,"Nessun repository disponibile!",(Vector2){(int)screenWidth/2 + 64, (int)screenHeight/2},(float)h2font.baseSize,-1,currentTheme.fgColor);
EndDrawing();
}

    while (atomic_load(&state.is_running)) {
    WaitTime(0.05);
    }
    for (size_t i = 0; i < state.count; i++) {
    free(state.repos[i].new_commits);
    }
    free(state.repos);

    UnloadFont(boldFont);
    UnloadFont(textFont);
    UnloadFont(h1font);
    UnloadFont(h2font);
    UnloadFont(h3font);
    UnloadFont(monoFont);
    CloseWindow();
    git_libgit2_shutdown();
return 0;
}