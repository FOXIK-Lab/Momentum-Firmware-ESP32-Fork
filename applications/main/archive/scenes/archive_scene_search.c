/* Momentum Search screen, adapted to the ESP32 archive worker/lifecycle. */
#include "../archive_i.h"
#include "../helpers/archive_browser.h"
#include <toolbox/dir_walk.h>
#include <strings.h>

#define ARCHIVE_SEARCH_LIMIT 2048U

void archive_search_stop(ArchiveBrowserView* browser) {
    atomic_store(&browser->search_cancel, true);
    if(browser->search_thread) {
        furi_thread_join(browser->search_thread);
        furi_thread_free(browser->search_thread);
        browser->search_thread = NULL;
    }
}

static int32_t archive_search_worker(void* context) {
    ArchiveApp* archive = context;
    ArchiveBrowserView* browser = archive->browser;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    DirWalk* walk = dir_walk_alloc(storage);
    FuriString* path = furi_string_alloc();
    FileInfo info;
    uint32_t count = 1;
    const char* notice = NULL;
    if(!dir_walk_open(walk, STORAGE_EXT_PATH_PREFIX)) {
        notice = "Error while searching!";
    } else {
        uint32_t visited = 0;
        while(!atomic_load(&browser->search_cancel)) {
            DirWalkResult result = dir_walk_read(walk, path, &info);
            if(result == DirWalkLast) break;
            if(result == DirWalkError) { notice = "Error while searching!"; break; }
            const char* name = strrchr(furi_string_get_cstr(path), '/');
            name = name ? name + 1 : furi_string_get_cstr(path);
            if(!file_info_is_dir(&info) &&
               strcasestr(name, archive->search_query) != NULL) {
                archive_add_file_item(browser, false, furi_string_get_cstr(path));
                archive_set_item_count(browser, ++count);
                if(count > ARCHIVE_SEARCH_LIMIT) {
                    notice = "Too many results, refine search";
                    break;
                }
            }
            if((++visited % 16) == 0) furi_delay_ms(1);
        }
    }
    if(count == 1 && !notice && !atomic_load(&browser->search_cancel)) notice = "No results found!";
    if(notice) {
        furi_string_printf(path, "/app:search/%s", notice);
        archive_add_app_item(browser, furi_string_get_cstr(path));
        archive_set_item_count(browser, ++count);
    }
    with_view_model(browser->view, ArchiveBrowserViewModel* model, {
        furi_string_set_str(files_array_get(model->files, 0)->path, "/app:search/Search for files");
        model->searching = false;
    }, true);
    furi_string_free(path);
    dir_walk_free(walk);
    furi_record_close(RECORD_STORAGE);
    return 0;
}

static void archive_search_input_done(void* context) {
    ArchiveApp* archive = context;
    view_dispatcher_send_custom_event(archive->view_dispatcher, 0);
}

void archive_scene_search_on_enter(void* context) {
    ArchiveApp* archive = context;
    archive_search_stop(archive->browser);
    archive->search_query[0] = '\0';
    text_input_set_header_text(archive->text_input, "Search for files:");
    text_input_set_result_callback(archive->text_input, archive_search_input_done, archive,
                                  archive->search_query, sizeof(archive->search_query), false);
    view_dispatcher_switch_to_view(archive->view_dispatcher, ArchiveViewTextInput);
}

void archive_search_start(ArchiveApp* archive) {
    archive_search_stop(archive->browser);
    if(archive->search_query[0] == '\0') return;
    ArchiveBrowserView* browser = archive->browser;
    archive_file_array_rm_all(browser);
    archive_add_app_item(browser, "/app:search/Cancel search");
    with_view_model(browser->view, ArchiveBrowserViewModel* model, {
        model->item_cnt = 1;
        model->item_idx = model->array_offset = model->list_offset = 0;
        model->searching = true;
    }, false);
    atomic_store(&browser->search_cancel, false);
    browser->search_thread = furi_thread_alloc_ex("ArchiveSearch", 4096, archive_search_worker, archive);
    furi_thread_start(browser->search_thread);
}

bool archive_scene_search_on_event(void* context, SceneManagerEvent event) {
    ArchiveApp* archive = context;
    if(event.type != SceneManagerEventTypeCustom || event.event != 0) return false;
    if(archive->search_query[0] == '\0') return true;
    archive_search_start(archive);
    scene_manager_previous_scene(archive->scene_manager);
    return true;
}

void archive_scene_search_on_exit(void* context) {
    ArchiveApp* archive = context;
    text_input_reset(archive->text_input);
}
