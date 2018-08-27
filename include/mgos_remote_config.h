#ifndef MGOS_REMOTE_CONFIG
#define MGOS_REMOTE_CONFIG

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "common/mg_str.h"
#include "frozen.h"
#include "mgos_event.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MGOS_REMOTE_CONFIG_BASE MGOS_EVENT_BASE('R', 'C', 'F')

typedef bool (*mgos_data_update)(void *data, const struct json_token *token);

/* In the comment, the type of `void *ev_data` is specified */
enum mgos_remote_config_event {
  MGOS_REMOTE_CONFIG_UPDATE_FULL =
      MGOS_REMOTE_CONFIG_BASE,   /* struct mg_str - remote config */
  MGOS_REMOTE_CONFIG_UPDATE_RAW, /* struct mgos_remote_config_update_raw */
  MGOS_REMOTE_CONFIG_UPDATE,     /* struct mgos_remote_config_update */
};

/* ev_data for MGOS_REMOTE_CONFIG_UPDATE event. */
struct mgos_remote_config_update {
  const char *path;
  void *value;
};

/* ev_data for MGOS_REMOTE_CONFIG_UPDATE_RAW event. */
struct mgos_remote_config_update_raw {
  const char *path;
  const struct json_token *token;
};

struct mgos_remote_config_data {
  void *data;
  mgos_data_update update;
};

struct mgos_remote_config_prop {
  const char *path;
  struct mgos_remote_config_data data;
};

void mgos_remote_config_register(struct mgos_remote_config_prop *props,
                                 size_t len);

struct mgos_remote_config_data mgos_remote_config_data_bool(bool defaultValue);
struct mgos_remote_config_data
mgos_remote_config_data_string(const char *defaultValue);
struct mgos_remote_config_data mgos_remote_config_data_int(int defaultValue);

bool mgos_remote_config_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_REMOTE_CONFIG */
