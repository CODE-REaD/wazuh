/* Copyright (C) 2015-2019, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
*/

#include "csyslogd-config.h"
#include "config.h"


int Read_CSyslog(XML_NODE node, void *config, __attribute__((unused)) void *config2, char **output)
{
    unsigned int i = 0, s = 0;
    char message[OS_FLSIZE];

    /* XML definitions */
    const char *xml_syslog_server = "server";
    const char *xml_syslog_port = "port";
    const char *xml_syslog_format = "format";
    const char *xml_syslog_level = "level";
    const char *xml_syslog_id = "rule_id";
    const char *xml_syslog_group = "group";
    const char *xml_syslog_location = "location";
    const char *xml_syslog_use_fqdn = "use_fqdn";

    struct SyslogConfig_holder *config_holder = (struct SyslogConfig_holder *)config;
    SyslogConfig **syslog_config = config_holder->data;

    if (syslog_config) {
        while (syslog_config[s]) {
            s++;
        }
    }

    /* Allocate the memory for the config */
    os_realloc(syslog_config, (s + 2) * sizeof(SyslogConfig *), syslog_config);
    os_calloc(1, sizeof(SyslogConfig), syslog_config[s]);
    syslog_config[s + 1] = NULL;

    /* Zero the elements */
    syslog_config[s]->server = NULL;
    syslog_config[s]->rule_id = NULL;
    syslog_config[s]->group = NULL;
    syslog_config[s]->location = NULL;
    syslog_config[s]->level = 0;
    syslog_config[s]->port = 514;
    syslog_config[s]->format = DEFAULT_CSYSLOG;
    syslog_config[s]->use_fqdn = 0;
    /* local 0 facility (16) + severity 4 - warning. --default */
    syslog_config[s]->priority = (16 * 8) + 4;

    while (node[i]) {
        if (!node[i]->element) {
            if (output == NULL) {
                merror(XML_ELEMNULL);
            } else {
                wm_strcat(output, "Invalid NULL element in the configuration.", '\n');
            }
            goto fail;
        } else if (!node[i]->content) {
            if (output == NULL) {
                merror(XML_VALUENULL, node[i]->element);
            } else {
                snprintf(message, OS_FLSIZE,
                        "Invalid NULL content for element: %s.",
                        node[i]->element);
                wm_strcat(output, message, '\n');
            }
            goto fail;
        } else if (strcmp(node[i]->element, xml_syslog_level) == 0) {
            if (!OS_StrIsNum(node[i]->content)) {
                if (output == NULL) {
                    merror(XML_VALUEERR, node[i]->element, node[i]->content);
                } else {
                    snprintf(message, OS_FLSIZE,
                        "Invalid value for element '%s': %s.",
                        node[i]->element, node[i]->content);
                    wm_strcat(output, message, '\n');
                }
                goto fail;
            }

            syslog_config[s]->level = (unsigned int) atoi(node[i]->content);
        } else if (strcmp(node[i]->element, xml_syslog_port) == 0) {
            if (!OS_StrIsNum(node[i]->content)) {
                if (output == NULL) {
                    merror(XML_VALUEERR, node[i]->element, node[i]->content);
                } else {
                    snprintf(message, OS_FLSIZE,
                        "Invalid value for element '%s': %s.",
                        node[i]->element, node[i]->content);
                    wm_strcat(output, message, '\n');
                }
                goto fail;
            }

            syslog_config[s]->port = (unsigned int) atoi(node[i]->content);
        } else if (strcmp(node[i]->element, xml_syslog_server) == 0) {
            os_strdup(node[i]->content, syslog_config[s]->server);
        } else if (strcmp(node[i]->element, xml_syslog_id) == 0) {
            unsigned int r_id = 0;
            char *str_pt = node[i]->content;

            while (*str_pt != '\0') {
                /* We allow spaces in between */
                if (*str_pt == ' ') {
                    str_pt++;
                    continue;
                }

                /* If is digit, we get the value
                 * and search for the next digit
                 * available
                 */
                else if (isdigit((int)*str_pt)) {
                    unsigned int id_i = 0;

                    r_id = (unsigned int) atoi(str_pt);
                    if (output == NULL) {
                        mdebug1("Adding '%d' to syslog alerting", r_id);
                    }

                    if (syslog_config[s]->rule_id) {
                        while (syslog_config[s]->rule_id[id_i]) {
                            id_i++;
                        }
                    }

                    os_realloc(syslog_config[s]->rule_id,
                               (id_i + 2) * sizeof(unsigned int),
                               syslog_config[s]->rule_id);

                    syslog_config[s]->rule_id[id_i + 1] = 0;
                    syslog_config[s]->rule_id[id_i] = r_id;

                    str_pt = strchr(str_pt, ',');
                    if (str_pt) {
                        str_pt++;
                    } else {
                        break;
                    }
                }

                /* Check for duplicate commas */
                else if (*str_pt == ',') {
                    str_pt++;
                    continue;
                }

                else {
                    break;
                }
            }

        } else if (strcmp(node[i]->element, xml_syslog_format) == 0) {
            if (strcmp(node[i]->content, "default") == 0) {
                /* Default is full format */
            } else if (strcmp(node[i]->content, "cef") == 0) {
                /* Enable the CEF format */
                syslog_config[s]->format = CEF_CSYSLOG;
            } else if (strcmp(node[i]->content, "json") == 0) {
                /* Enable the JSON format */
                syslog_config[s]->format = JSON_CSYSLOG;
            } else if (strcmp(node[i]->content, "splunk") == 0) {
                /* Enable the Splunk Key/Value format */
                syslog_config[s]->format = SPLUNK_CSYSLOG;
            } else if (output == NULL) {
                merror(XML_VALUEERR, node[i]->element, node[i]->content);
                goto fail;
            } else {
                snprintf(message, OS_FLSIZE,
                    "Invalid value for element '%s': %s.",
                    node[i]->element, node[i]->content);
                wm_strcat(output, message, '\n');
                goto fail;
            }
        } else if (strcmp(node[i]->element, xml_syslog_location) == 0) {
            os_calloc(1, sizeof(OSMatch), syslog_config[s]->location);
            if (!OSMatch_Compile(node[i]->content,
                                 syslog_config[s]->location, 0)) {
                if (output == NULL) {
                    merror(REGEX_COMPILE, node[i]->content,
                       syslog_config[s]->location->error);
                } else {
                    snprintf(message, OS_FLSIZE,
                        "Syntax error on regex: '%s': %d.",
                        node[i]->content, syslog_config[s]->location->error);
                    wm_strcat(output, message, '\n');
                }
                goto fail;
            }
        } else if (strcmp(node[i]->element, xml_syslog_use_fqdn) == 0) {
            if (strcmp(node[i]->content, "yes") == 0) {
                syslog_config[s]->use_fqdn = 1;
            } else if (strcmp(node[i]->content, "no") == 0) {
                syslog_config[s]->use_fqdn = 0;
            } else if (output == NULL) {
                merror(XML_VALUEERR, node[i]->element, node[i]->content);
                goto fail;
            } else {
                snprintf(message, OS_FLSIZE,
                    "Invalid value for element '%s': %s.",
                    node[i]->element, node[i]->content);
                wm_strcat(output, message, '\n');
                goto fail;
            }
        } else if (strcmp(node[i]->element, xml_syslog_group) == 0) {
            os_calloc(1, sizeof(OSMatch), syslog_config[s]->group);
            if (!OSMatch_Compile(node[i]->content,
                                 syslog_config[s]->group, 0)) {
                if (output == NULL){
                    merror(REGEX_COMPILE, node[i]->content,
                       syslog_config[s]->group->error);
                } else {
                    snprintf(message, OS_FLSIZE,
                        "Syntax error on regex: '%s': %d.",
                        node[i]->content, syslog_config[s]->group->error);
                    wm_strcat(output, message, '\n');
                }
                goto fail;
            }
        } else if (output == NULL) {
            merror(XML_INVELEM, node[i]->element);
            goto fail;
        } else {
            snprintf(message, OS_FLSIZE,
                "Invalid element in the configuration: '%s'.",
                node[i]->element);
            wm_strcat(output, message, '\n');
            goto fail;
        }
        i++;
    }

    /* We must have at least one entry set */
    if (!syslog_config[s]->server) {
        if (output == NULL){
            merror(XML_INV_CSYSLOG);
        } else {
            wm_strcat(output, "Invalid client-syslog configuration.", '\n');
        }
        goto fail;
    }

    config_holder->data = syslog_config;
    return (0);

fail:
    i = 0;
    while (syslog_config[i]) {
        free(syslog_config[i]->server);

        if (syslog_config[i]->group) {
            OSMatch_FreePattern(syslog_config[i]->group);
        }

        if (syslog_config[i]->location) {
            OSMatch_FreePattern(syslog_config[i]->location);
        }

        free(syslog_config[i]->rule_id);

        ++i;
    }
    free(syslog_config);
    return (OS_INVALID);
}

int Test_CSyslogd(const char *path, char **output) {
    int fail = 0;

    struct SyslogConfig_holder config;
    SyslogConfig **syslog_config = NULL;

    config.data = syslog_config;

    if (ReadConfig(CSYSLOGD, path, &config, NULL, output) < 0) {
        if (output == NULL){
            merror(RCONFIG_ERROR,"CSyslogd", path);
        } else {
            wm_strcat(output, "ERROR: Invalid configuration in CSyslogd", '\n');
        }
        fail = 1;
    }

    // Free Memory
    free_csyslog_holder(&config);

    if(fail) {
        return -1;
    }

    return 0;
}

void free_csyslog_holder(struct SyslogConfig_holder * config) {
    if(config) {
        if(config->data) {
            int i;
            for (i = 0; config->data[i]; i++) {
                os_free(config->data[i]->server);
                os_free(config->data[i]->rule_id);
                os_free(config->data[i]->location);
                os_free(config->data[i]->group);
                free(config->data[i]);
            }
        }
        os_free(config->data);
    }
}
