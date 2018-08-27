/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos_remote_config.h"
#include "common/cs_dbg.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"
#include "mgos_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct mgos_remote_data_node {
  void *data;
  const char *path;
  mgos_data_update update;
};

struct mgos_remote_data {
  bool enabled;
  bool inited;
  struct mgos_rlock_type *lock;
  struct mgos_remote_data_node *nodes;
  uint8_t len;
};

static struct mgos_remote_data _data;

void mgos_remote_config_register(struct mgos_remote_config_prop *props,
                                 size_t len) {
  if (!_data.enabled) {
    return;
  }

  if (_data.inited) {
    LOG(LL_WARN,
        ("mgos_remote_config_register called after init, will be ignored"));
  }

  mgos_rlock(_data.lock);
  struct mgos_remote_data_node *new_nodes =
      malloc(sizeof(struct mgos_remote_data_node) * (len + _data.len));

  if (_data.len > 0) {
    memcpy(new_nodes, _data.nodes,
           sizeof(struct mgos_remote_data_node) * _data.len);
  }

  for (size_t i = 0; i < len; i++) {
    struct mgos_remote_config_prop *prop = props + i;
    struct mgos_remote_data_node node;
    node.path = prop->path;
    node.data = prop->data.data;
    node.update = prop->data.update;
    new_nodes[_data.len + i] = node;
  }

  if (_data.nodes != NULL) {
    free(_data.nodes);
  }

  _data.nodes = new_nodes;
  _data.len += len;
  mgos_runlock(_data.lock);
}

struct mgos_remote_config_walk_data {
  bool trigger_events;
  struct mgos_remote_config_update_raw raw_ev;
  struct mgos_remote_config_update ev;
};

void mgos_remote_config_json_walk(void *callback_data, const char *name,
                                  size_t name_len, const char *path,
                                  const struct json_token *token) {
  struct mgos_remote_config_walk_data *data =
      (struct mgos_remote_config_walk_data *)callback_data;

  switch (token->type) {
    case JSON_TYPE_STRING:
    case JSON_TYPE_NUMBER:
    case JSON_TYPE_TRUE:
    case JSON_TYPE_FALSE:
    case JSON_TYPE_NULL:
    case JSON_TYPE_OBJECT_END:
    case JSON_TYPE_ARRAY_END: {
      if (data->trigger_events) {
        data->raw_ev.path = path;
        data->raw_ev.token = token;
        mgos_event_trigger(MGOS_REMOTE_CONFIG_UPDATE_RAW, &data->raw_ev);
      }

      for (int i = 0; i < _data.len; i++) {
        if (strcmp(path, _data.nodes[i].path) == 0) {
          bool wasUpdated = _data.nodes[i].update(_data.nodes[i].data, token);
          if (wasUpdated && data->trigger_events) {
            data->ev.path = path;
            data->ev.value = _data.nodes[i].data;
            mgos_event_trigger(MGOS_REMOTE_CONFIG_UPDATE, &data->ev);
          }
        }
      }
      break;
    }

    default:
      break;
  }

  (void)name;
  (void)name_len;
}

void mgos_remote_config_save(struct mg_str *json_string) {
  FILE *fs;
  if ((fs = fopen("remote_config.json", "w"))) {
    fwrite(json_string->p, sizeof(char), json_string->len, fs);
    fclose(fs);
  }
}

void mgos_remote_config_update_full(int ev, void *ev_data, void *userdata) {
  struct mg_str *json_string = (struct mg_str *)ev_data;

  // Step 1: Write to disk
  mgos_remote_config_save(json_string);

  // Step 2: Call update events
  struct mgos_remote_config_walk_data *walk_data = malloc(sizeof(*walk_data));
  walk_data->trigger_events = true;
  json_walk(json_string->p, json_string->len, mgos_remote_config_json_walk,
            walk_data);
  free(walk_data);

  (void)ev;
  (void)userdata;
}

void mgos_remote_config_init_done(int ev, void *ev_data, void *userdata) {
  const char *json_string = json_fread("remote_config.json");
  if (json_string) {
    struct mgos_remote_config_walk_data *walk_data = malloc(sizeof(*walk_data));
    walk_data->trigger_events = false;
    json_walk(json_string, strlen(json_string), mgos_remote_config_json_walk,
              walk_data);
    free(walk_data);
  }

  (void)ev;
  (void)ev_data;
  (void)userdata;
}

bool mgos_remote_config_init(void) {
  if (!mgos_sys_config_get_rcfg_enable()) {
    _data.enabled = false;
    return true;
  }

  mgos_event_register_base(MGOS_REMOTE_CONFIG_BASE, "rcfg");
  _data.enabled = true;
  _data.inited = false;
  _data.lock = mgos_rlock_create();
  _data.len = 0;
  _data.nodes = NULL;
  mgos_event_add_handler(MGOS_REMOTE_CONFIG_UPDATE_FULL,
                         mgos_remote_config_update_full, NULL);
  mgos_event_add_handler(MGOS_EVENT_INIT_DONE, mgos_remote_config_init_done,
                         NULL);
  return true;
}
