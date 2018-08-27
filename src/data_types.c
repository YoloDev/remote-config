#include "common/cs_dbg.h"
#include "common/mg_mem.h"
#include "mgos_remote_config.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"
#include "mgos_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool update_bool(void *store, const struct json_token *token,
                        const char *path) {
  bool *data = (bool *)store;
  bool newVal;
  switch (token->type) {
    case JSON_TYPE_TRUE: {
      newVal = true;
      break;
    }

    case JSON_TYPE_FALSE: {
      newVal = false;
      break;
    }

    default: {
      LOG(LL_WARN,
          ("%s: expected bool, but was: %.*s", path, token->len, token->ptr));
      return false;
    }
  }

  if (newVal != *data) {
    *data = newVal;
    return true;
  }

  return false;
}

struct mut_str {
  char *p;
  struct mg_str str;
};

static struct mut_str mut_strdup(const struct mg_str s) {
  struct mut_str r = {NULL, {NULL, 0}};
  if (s.len > 0 && s.p != NULL) {
    char *sc = (char *)MG_MALLOC(s.len);
    if (sc != NULL) {
      memcpy(sc, s.p, s.len);
      r.p = sc;
      r.str.p = sc;
      r.str.len = s.len;
    }
  }
  return r;
}

static bool update_string(void *store, const struct json_token *token,
                          const char *path) {
  struct mut_str *data = (struct mut_str *)store;
  switch (token->type) {
    case JSON_TYPE_STRING: {
      struct mg_str token_value = mg_mk_str_n(token->ptr, token->len);
      if (token_value.len != data->str.len ||
          mg_strcmp(token_value, data->str) != 0) {
        struct mut_str new_value = mut_strdup(token_value);
        MG_FREE(data->p);
        *data = new_value;
        return true;
      }

      return false;
    }

    default: {
      LOG(LL_WARN,
          ("%s: expected string, but was: %.*s", path, token->len, token->ptr));
      return false;
    }
  }
}

static bool update_int(void *store, const struct json_token *token,
                       const char *path) {
  int *data = (int *)store;
  switch (token->type) {
    case JSON_TYPE_NUMBER: {
      int new_value = (int)strtol(token->ptr, (char **)NULL, 10);
      if (new_value != *data) {
        *data = new_value;
        return true;
      }

      return false;
    }

    default: {
      LOG(LL_WARN,
          ("%s: expected number, but was: %.*s", path, token->len, token->ptr));
      return false;
    }
  }
}

struct mgos_remote_config_data mgos_remote_config_data_bool(bool defaultValue) {
  bool *data = malloc(sizeof(bool));
  *data = defaultValue;
  struct mgos_remote_config_data ret = {.data = data, .update = update_bool};
  return ret;
}

struct mgos_remote_config_data
mgos_remote_config_data_string(const char *defaultValue) {
  struct mut_str *data = malloc(sizeof(struct mut_str));
  *data = mut_strdup(mg_mk_str(defaultValue));
  struct mgos_remote_config_data ret = {.data = data, .update = update_string};
  return ret;
}

struct mgos_remote_config_data mgos_remote_config_data_int(int defaultValue) {
  int *data = malloc(sizeof(int));
  *data = defaultValue;
  struct mgos_remote_config_data ret = {.data = data, .update = update_int};
  return ret;
}
