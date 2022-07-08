/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2019 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Helper functions for build configuration files
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#include "tsapi-conf.h"
#include "te_string.h"
#include "rcf_api.h"

/* See description in tsapi-conf.h */
tsapi_conf_section *
tsapi_conf_section_get(tsapi_conf_t *conf, const char *section_name)
{
    tsapi_conf_section *section;

    TE_VEC_FOREACH(&conf->sections, section)
    {
        if (strcmp(section->name, section_name) == 0)
        {
            return section;
        }
    }

    return NULL;
}

/* See description in tsapi-conf.h */
te_errno
tsapi_conf_create_section(tsapi_conf_t *conf, const char *section_name,
                          const char *delimiter)
{
    tsapi_conf_section new;

    new.name = strdup(section_name);
    new.delimiter = strdup(delimiter);
    new.items = TE_VEC_INIT(tsapi_conf_kv);

    return TE_VEC_APPEND(&conf->sections, new);
}

/* See description in tsapi-conf.h */
te_errno
tsapi_conf_append(tsapi_conf_t *conf, const char *section_name,
                  const char *key, const char *value)
{
    tsapi_conf_section *section = NULL;
    tsapi_conf_kv item;

    if (conf == NULL)
    {
        ERROR("Invalid config handler");
        return TE_EINVAL;
    }

    section = tsapi_conf_section_get(conf, section_name);
    if (section == NULL)
    {
        ERROR("Failed to append conf to nonexistent section %s", section_name);
        return TE_EFAIL;
    }

    item.key = strdup(key);
    if (item.key == NULL)
        return TE_ENOMEM;

    item.value = strdup(value);
    if (item.value == NULL)
        return TE_ENOMEM;

    return TE_VEC_APPEND(&section->items, item);
}

/* See description in tsapi-conf.h */
void
tsapi_conf_free(tsapi_conf_t *conf)
{
    tsapi_conf_section *section;
    tsapi_conf_kv *kv;

    TE_VEC_FOREACH(&conf->sections, section)
    {
        TE_VEC_FOREACH(&section->items, kv)
        {
            free(kv->key);
            free(kv->value);
        }

        free(section->name);
        free(section->delimiter);
        te_vec_free(&section->items);
    }

    te_vec_free(&conf->sections);
}

/**
 * Flush conf handler to string
 *
 * @param conf Conf handler
 * @param str  String to flush config
 *
 * @return Status code
 */
static te_errno
flush_conf_to_str(tsapi_conf_t *conf, te_string *str)
{
    tsapi_conf_section *section;
    tsapi_conf_kv *item;
    te_errno rc = 0;

    TE_VEC_FOREACH(&conf->sections, section)
    {
        rc = te_string_append(str, "[%s]\n", section->name);

        if (rc != 0)
            break;

        TE_VEC_FOREACH(&section->items, item)
        {
            rc = te_string_append(str, "%s %s %s\n",
                                  item->key, section->delimiter,
                                  item->value);

            if (rc != 0)
                break;
        }
    }

    if (rc != 0)
    {
        ERROR("Failed to flush config %r", rc);
        te_string_free(str);
    }

    return rc;
}

/* See description in tsapi-conf.h */
te_errno
tsapi_conf_flush(rcf_rpc_server *rpcs, tsapi_conf_t *conf,
                 const char *path)
{
    te_errno rc;
    te_string config_str = TE_STRING_INIT;

    rc = flush_conf_to_str(conf, &config_str);
    if (rc != 0)
    {
        te_string_free(&config_str);
        return rc;
    }

    rc = tsapi_put_str_to_agt_file(rpcs, &config_str, path);
    te_string_free(&config_str);

    return rc;
}

te_errno
tsapi_conf_flush_local(tsapi_conf_t *conf, const char *path)
{
    te_errno rc;
    te_string config_str = TE_STRING_INIT;

    rc = flush_conf_to_str(conf, &config_str);
    if (rc != 0)
    {
        ERROR("Failed to flush config: %r", rc);
        te_string_free(&config_str);
        return rc;
    }

    rc = tsapi_put_str_to_local_file(&config_str, path);
    if (rc != 0)
        ERROR("Failed to create config file: %r", rc);

    te_string_free(&config_str);

    return rc;
}

/* See description in tsapi-conf.h */
te_errno
tsapi_conf_opts_read(int argc, char **argv, const char *prefix, te_vec *vec)
{
    int i;
    te_errno rc;

    for (i = 0; i < argc; i++)
    {
        if (strcmp_start(prefix, argv[i]) != 0)
            continue;

        rc = te_vec_append_str_fmt(vec, "%s", argv[i] + strlen(prefix) + 1);
        if (rc != 0)
            return rc;
    }

    if (te_vec_size(vec) == 0)
    {
        ERROR("There is no string starts with %s", prefix);
        return TE_EFAIL;
    }

    return 0;
}

/**
 * Extract section name from string in "Section%sKey=Value" format,
 * where %s - @p delim
 *
 * @param str          Configuration string
 * @param delim        Delimiter between section name and key
 * @param section_name The string to save section name
 *
 * @return Statuc code
 */
static te_errno
extract_section_name(const char *str, const char *delim, te_string *section_name)
{
    char *ch;
    te_errno rc;

    ch = strstr(str, delim);
    if (ch == NULL)
    {
        ERROR("Invalid configuartion string format");
        return TE_EFAIL;
    }

    rc = te_string_append(section_name, "%s", str);
    if (rc != 0)
        return rc;

    te_string_cut(section_name, strlen(str) - (ch - str));

    return 0;
}

/**
 * Extract key-value pair from string in "Section-Key%sValue" format,
 * where %s - @p delim
 *
 * @param str          Configuration string
 * @param delim        Key-value delimiter
 * @param section_name key-value pair structure to save
 *
 * @return Statuc code
 */
static te_errno
extract_key_value(const char *str, const char *delim, tsapi_conf_kv *kv)
{
    char *ch;

    ch = strstr(str, delim);
    if (ch == NULL)
    {
        ERROR("Invalid configuartion string format");
        return TE_EFAIL;
    }

    kv->key = TE_ALLOC(ch - str);
    strncpy(kv->key, str, ch - str);

    kv->value = strdup(ch + 1);
    if (kv->value == NULL)
        return TE_ENOMEM;

    return 0;
}

/* See description in tsapi-conf.h */
te_errno
tsapi_conf_parse_string(const char *str, te_string *section_name,
                        tsapi_conf_kv *kv)
{
    te_errno rc;

    rc = extract_section_name(str, "-", section_name);
    if (rc != 0)
    {
        ERROR("Failed to extract section name from configuration string: %s "
              "rc = %r", str, rc);

        return rc;
    }

    rc = extract_key_value(str + section_name->len + 1, "=", kv);
    if (rc != 0)
    {
        ERROR("Failed to extract key-value from configuration string: %s "
              "rc = %r", str, rc);
    }

    return rc;
}

/* See description in tsapi-conf.h */
te_errno
tsapi_conf_read_from_file(const char *ta, const char *filename,
                          tsapi_conf_t *conf)
{
    FILE *f;
    te_string local_conf_path = TE_STRING_INIT;
    char buf[PATH_MAX];
    char *begin_str = NULL;
    tsapi_conf_kv kv;
    te_string section_name = TE_STRING_INIT;
    te_string conf_str = TE_STRING_INIT;
    te_errno rc;

    te_string_append(&local_conf_path, "%s/bp.conf", getenv("TE_TMP"));

    rc = rcf_ta_get_file(ta, 0, filename, local_conf_path.ptr);
    if (rc != 0)
    {
        ERROR("Failed to get file %s from the '%s'", local_conf_path.ptr,
              ta);
        te_string_free(&local_conf_path);
        return rc;
    }

    f = fopen(local_conf_path.ptr, "r");
    if (f == NULL)
    {
        ERROR("Failed to open %s", local_conf_path.ptr);
        te_string_free(&local_conf_path);
        return te_rc_os2te(errno);
    }

    while (fgets(buf, PATH_MAX, f) != NULL)
    {
        if (buf[0] == '[')
        {
            te_string_reset(&section_name);
            rc = extract_section_name(buf + 1, "]", &section_name);
            if (rc != 0)
                goto out;

            rc = tsapi_conf_create_section(conf, section_name.ptr, " ");
            if (rc != 0)
                goto out;
        }
        else
        {
            begin_str = te_str_strip_spaces(buf);
            rc = extract_key_value(begin_str, " ", &kv);
            if (rc != 0)
                goto out;

            rc = tsapi_conf_append(conf, section_name.ptr, kv.key, kv.value);
            if (rc != 0)
                goto out;
        }
    }

    rc = flush_conf_to_str(conf, &conf_str);

out:
    fclose(f);
    te_string_free(&local_conf_path);
    te_string_free(&section_name);

    return rc;
}
