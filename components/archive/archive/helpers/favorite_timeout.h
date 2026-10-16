#pragma once

#include <stdbool.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>

bool archive_process_favorite_launch(const char** args);
void archive_favorite_timeout_run(
    ViewDispatcher* dispatcher, SceneManager* scene_manager, bool favorite);
