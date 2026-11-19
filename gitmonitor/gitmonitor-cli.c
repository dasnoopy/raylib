#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <git2.h>

#define REPOS_LIST_FILE "repos.txt"
#define STATE_FILE      "repo_state.txt"
#define CACHE_DIR       ".cache_repos"

#define MAX_URL_LEN     256
#define MAX_MSG_LEN     128
#define MAX_AUTHOR_LEN  64
#define HASH_LEN        41
#define FETCH_DEPTH     20 // Scarica solo gli ultimi 20 commit per risparmiare banda

typedef struct {
    char hash[8];
    char author[MAX_AUTHOR_LEN];
    char message[MAX_MSG_LEN];
} CommitInfo;

typedef struct {
    char url[MAX_URL_LEN];
    char branch[32];
    char last_hash[HASH_LEN];
    char current_hash[HASH_LEN];
    int new_commits_count;
    CommitInfo *new_commits; // Array dinamico dei nuovi commit trovati
    bool is_new_repo;
    bool has_updates;
    bool error;
    char error_msg[128];
} RepoStatus;

// Genera un nome cartella sicuro (hash djb2 dell'URL) dentro .cache_repos
void get_cache_path(const char *url, char *out_path, size_t max_len) {
    unsigned long hash = 5381;
    int c;
    const char *str = url;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    snprintf(out_path, max_len, "%s/%lx.git", CACHE_DIR, hash);
}

// Carica gli URL dal file repos.txt ignorando righe vuote o commenti (#)
RepoStatus* load_repos_from_file(const char *filename, size_t *out_count) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        // Se non esiste, crea un file di esempio
        f = fopen(filename, "w");
        if (f) {
            fprintf(f, "# Inserisci un repository GitHub pubblico per riga\n");
            fprintf(f, "https://github.com\n");
            fprintf(f, "https://github.com\n");
            fclose(f);
        }
        f = fopen(filename, "r");
        if (!f) return NULL;
    }

    RepoStatus *list = NULL;
    size_t count = 0;
    char line[MAX_URL_LEN];

    while (fgets(line, sizeof(line), f)) {
        // Rimuove \n e \r
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '\0' || line[0] == '#') continue;

        RepoStatus *tmp = realloc(list, (count + 1) * sizeof(RepoStatus));
        if (!tmp) break;
        list = tmp;

        memset(&list[count], 0, sizeof(RepoStatus));
        strncpy(list[count].url, line, MAX_URL_LEN - 1);
        count++;
    }

    fclose(f);
    *out_count = count;
    return list;
}

void load_previous_hash(const char *url, char *out_hash) {
    out_hash[0] = '\0';
    FILE *f = fopen(STATE_FILE, "r");
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
    FILE *f = fopen(STATE_FILE, "w");
    if (!f) return;

    for (size_t i = 0; i < count; i++) {
        const char *hash_to_save = repos[i].error ? repos[i].last_hash : repos[i].current_hash;
        if (hash_to_save[0] != '\0') {
            fprintf(f, "%s %s\n", repos[i].url, hash_to_save);
        }
    }
    fclose(f);
}

// Estrae i commit tra new_oid e old_oid (se old_oid è NULL, prende solo l'ultimo)
void collect_commit_details(git_repository *repo, RepoStatus *status, const git_oid *new_oid, const git_oid *old_oid) {
    git_revwalk *walker = NULL;
    if (git_revwalk_new(&walker, repo) < 0) return;

    git_revwalk_sorting(walker, GIT_SORT_TIME);
    git_revwalk_push(walker, new_oid);

    if (old_oid != NULL) {
        // Nasconde il vecchio commit e tutti i suoi antenati: ci fermiamo esattamente lì!
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

                git_oid_tostr(info->hash, sizeof(info->hash), &oid);

                const git_signature *author = git_commit_author(commit);
                strncpy(info->author, author ? author->name : "Sconosciuto", MAX_AUTHOR_LEN - 1);
                info->author[MAX_AUTHOR_LEN - 1] = '\0';

                // Prende solo la prima riga (summary) del messaggio di commit
                const char *summary = git_commit_summary(commit);
                strncpy(info->message, summary ? summary : "", MAX_MSG_LEN - 1);
                info->message[MAX_MSG_LEN - 1] = '\0';

                status->new_commits_count++;
            }
            git_commit_free(commit);
        }

        // Se è un repo appena aggiunto, ci fermiamo dopo aver letto solo l'ultimo commit
        if (old_oid == NULL) break;
    }

    git_revwalk_free(walker);
}

void check_repository(RepoStatus *status) {
    git_remote *remote = NULL;

    // 1. Crea un remote "in memoria" (detached) senza ancora toccare il disco
    if (git_remote_create_detached(&remote, status->url) < 0) {
        status->error = true;
        const git_error *e = git_error_last();
        snprintf(status->error_msg, sizeof(status->error_msg), "%s", e ? e->message : "URL non valido");
        return;
    }

    // 2. FAST-CHECK: Connettiti solo per leggere i puntatori (nessun packfile scaricato)
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

    // Cerca l'OID remoto (priorità a main, fallback su master)
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

    // Chiudi subito la connessione leggera
    git_remote_disconnect(remote);
    git_remote_free(remote);

    if (!found) {
        status->error = true;
        snprintf(status->error_msg, sizeof(status->error_msg), "Branch main/master non trovato");
        return;
    }

    git_oid_tostr(status->current_hash, HASH_LEN, &remote_oid);

    // 3. SE L'HASH È UGUALE AL PRECEDENTE -> USCITA IMMEDIATA (Zero I/O su disco!)
    if (status->last_hash[0] != '\0' && strcmp(status->last_hash, status->current_hash) == 0) {
        status->has_updates = false;
        return;
    }

    // 4. Ci sono nuovi commit (o è un repo nuovo): apri/crea la cache su disco e fai il fetch
    char cache_path[512];
    get_cache_path(status->url, cache_path, sizeof(cache_path));

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

    // Scarica solo il branch effettivamente trovato (invece di provarli entrambi)
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

    // 5. Estrai i dettagli dei commit
    if (status->last_hash[0] == '\0') {
        status->is_new_repo = true;
        status->has_updates = true;
        collect_commit_details(repo, status, &remote_oid, NULL);
    } else {
        status->has_updates = true;
        git_oid old_oid;
        if (git_oid_fromstr(&old_oid, status->last_hash) == 0) {
            collect_commit_details(repo, status, &remote_oid, &old_oid);
        }
    }

    git_remote_free(repo_remote);
    git_repository_free(repo);
}


int main(void) {
    mkdir(CACHE_DIR, 0755);
    git_libgit2_init();

    size_t repo_count = 0;
    RepoStatus *repos = load_repos_from_file(REPOS_LIST_FILE, &repo_count);

    if (!repos || repo_count == 0) {
        printf("Nessun repository trovato in %s\n", REPOS_LIST_FILE);
        git_libgit2_shutdown();
        return 1;
    }

    printf("Controllo di %zu repository da '%s'...\n\n", repo_count, REPOS_LIST_FILE);

    for (size_t i = 0; i < repo_count; i++) {
        load_previous_hash(repos[i].url, repos[i].last_hash);
        check_repository(&repos[i]);

        if (repos[i].error) {
            printf("[ERRORE] %s -> %s\n", repos[i].url, repos[i].error_msg);
        } else if (repos[i].is_new_repo) {
            printf("[NUOVO REPO] %s (%s)\n", repos[i].url, repos[i].branch);
            if (repos[i].new_commits_count > 0) {
                printf("   Ultimo commit: [%s] %s - %s\n",
                       repos[i].new_commits[0].hash,
                       repos[i].new_commits[0].author,
                       repos[i].new_commits[0].message);
            }
        } else if (repos[i].has_updates) {
            printf("[AGGIORNATO!] %s (%s) -> %d nuovi commit:\n",
                   repos[i].url, repos[i].branch, repos[i].new_commits_count);
            for (int c = 0; c < repos[i].new_commits_count; c++) {
                printf("   * [%s] %s: %s\n",
                       repos[i].new_commits[c].hash,
                       repos[i].new_commits[c].author,
                       repos[i].new_commits[c].message);
            }
        } else {
            printf("[OK] %s (%s @ %.7s) - Nessun nuovo commit\n",
                   repos[i].url, repos[i].branch, repos[i].current_hash);
        }
    }

    save_current_state(repos, repo_count);

    // Pulizia memoria
    for (size_t i = 0; i < repo_count; i++) {
        free(repos[i].new_commits);
    }
    free(repos);
    git_libgit2_shutdown();
    return 0;
}
