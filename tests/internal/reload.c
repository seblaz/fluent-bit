/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <fluent-bit/flb_info.h>
#include <fluent-bit/flb_mem.h>
#include <fluent-bit/flb_kv.h>
#include <fluent-bit/flb_config_format.h>
#include <fluent-bit/flb_lib.h>
#include <fluent-bit/flb_reload.h>

#include <cfl/cfl.h>
#include <cfl/cfl_variant.h>
#include <cfl/cfl_kvlist.h>

#include "flb_tests_internal.h"

#define FLB_000 FLB_TESTS_DATA_PATH "/data/config_format/yaml/fluent-bit.yaml"

void test_reconstruct_cf()
{
    struct flb_cf *cf;
    struct flb_cf_section *s_tmp;
    struct flb_cf_section *service;
    struct flb_cf_group *g_tmp;
    struct cfl_variant *ret;
    struct flb_kv *meta;
    struct flb_cf *new_cf;
    int status;


    /* create context */
    cf = flb_cf_create();
    TEST_CHECK(cf != NULL);

    /* create service section */
    service = flb_cf_section_create(cf, "SERVICE", 7);
    TEST_CHECK(service != NULL);

    /* add a property */
    ret = flb_cf_section_property_add(cf, service->properties, "key", 3, "val", 3);
    TEST_CHECK(ret != NULL);

    /* add a property with empty spaces on left/right */
    ret = flb_cf_section_property_add(cf, service->properties, " key ", 5, " val   ", 7);
    TEST_CHECK(ret != NULL);

    /* add an invalid property */
    ret = flb_cf_section_property_add(cf, service->properties, "   ", 3, "", 0);
    TEST_CHECK(ret == NULL);

    /* try to add another 'SERVICE' section, it should return the same one */
    s_tmp = flb_cf_section_create(cf, "SERVICE", 7);
    TEST_CHECK(s_tmp == service);

    /* add a valid section */
    s_tmp = flb_cf_section_create(cf, "INPUT", 5);
    TEST_CHECK(s_tmp != NULL);

    TEST_CHECK(mk_list_size(&cf->inputs) == 1);

    /* add property to the section recently created */
    ret = flb_cf_section_property_add(cf, s_tmp->properties, "key", 3, "val", 3);
    TEST_CHECK(ret != NULL);

    /* groups: add groups to the last section created */
    g_tmp = flb_cf_group_create(cf, s_tmp, "FLUENT GROUP", 12);
    TEST_CHECK(g_tmp != NULL);

    /* add properties to the group */
    ret = flb_cf_section_property_add(cf, g_tmp->properties, "key", 3, "val", 3);
    TEST_CHECK(ret != NULL);

    /* groups: invalid group */
    g_tmp = flb_cf_group_create(cf, s_tmp, "", 0);
    TEST_CHECK(g_tmp == NULL);

    /* Meta commands */
    meta = flb_cf_meta_property_add(cf, "@SET        a=1     ", 20);

    TEST_CHECK(meta != NULL);
    TEST_CHECK(flb_sds_len(meta->key) == 3 && strcmp(meta->key, "SET") == 0);
    TEST_CHECK(flb_sds_len(meta->val) == 3 && strcmp(meta->val, "a=1") == 0);

    /* create new context */
    new_cf = flb_cf_create();
    TEST_CHECK(cf != NULL);

    status = flb_reload_reconstruct_cf(cf, new_cf);
    TEST_CHECK(status == 0);
    TEST_CHECK(new_cf != NULL);

    TEST_CHECK(mk_list_size(&new_cf->inputs) == 1);
    TEST_CHECK(mk_list_size(&new_cf->sections) == 2);
    TEST_CHECK(mk_list_size(&new_cf->metas) == 1);

    printf("\n");
    flb_cf_dump(new_cf);

    /* destroy context */
    flb_cf_destroy(cf);
    flb_cf_destroy(new_cf);
}


/* data/config_format/fluent-bit.yaml */
void test_reconstruct_cf_yaml()
{
    struct mk_list *head;
    struct flb_cf *cf;
    struct flb_cf_section *s;
    struct flb_cf_group *g;
    int status;
    struct flb_cf *new_cf;

    cf = flb_cf_yaml_create(NULL, FLB_000, NULL, 0);
    TEST_CHECK(cf != NULL);
    if (!cf) {
        exit(EXIT_FAILURE);
    }

    /* Total number of sections */
    TEST_CHECK(mk_list_size(&cf->sections) == 9);

    /* SERVICE check */
    TEST_CHECK(cf->service != NULL);
    if (cf->service) {
        TEST_CHECK(cfl_list_size(&cf->service->properties->list) == 3);
    }

    /* Check number sections per list */
    TEST_CHECK(mk_list_size(&cf->parsers) == 0);
    TEST_CHECK(mk_list_size(&cf->multiline_parsers) == 0);
    TEST_CHECK(mk_list_size(&cf->customs) == 1);
    TEST_CHECK(mk_list_size(&cf->inputs) == 3);
    TEST_CHECK(mk_list_size(&cf->filters) == 1);
    TEST_CHECK(mk_list_size(&cf->outputs) == 2);
    TEST_CHECK(mk_list_size(&cf->others) == 1);
    TEST_CHECK(mk_list_size(&cf->env) == 2);

    /* groups */
    s = flb_cf_section_get_by_name(cf, "input");
    TEST_CHECK(s != NULL);
    TEST_CHECK(mk_list_size(&s->groups) == 1);

    mk_list_foreach(head, &s->groups) {
        g = mk_list_entry(head, struct flb_cf_group, _head);
        TEST_CHECK(cfl_list_size(&g->properties->list) == 2);
    }

    /* create new context */
    new_cf = flb_cf_create();
    TEST_CHECK(cf != NULL);

    status = flb_reload_reconstruct_cf(cf, new_cf);
    TEST_CHECK(status == 0);
    TEST_CHECK(new_cf != NULL);

    /* Total number of sections */
    TEST_CHECK(mk_list_size(&cf->sections) == 9);

    /* SERVICE check */
    TEST_CHECK(cf->service != NULL);
    if (cf->service) {
        TEST_CHECK(cfl_list_size(&cf->service->properties->list) == 3);
    }

    /* Check number sections per list */
    TEST_CHECK(mk_list_size(&new_cf->parsers) == 0);
    TEST_CHECK(mk_list_size(&new_cf->multiline_parsers) == 0);
    TEST_CHECK(mk_list_size(&new_cf->customs) == 1);
    TEST_CHECK(mk_list_size(&new_cf->inputs) == 3);
    TEST_CHECK(mk_list_size(&new_cf->filters) == 1);
    TEST_CHECK(mk_list_size(&new_cf->outputs) == 2);
    TEST_CHECK(mk_list_size(&new_cf->others) == 1);
    TEST_CHECK(mk_list_size(&cf->env) == 2);

    printf("\n");
    flb_cf_dump(new_cf);
    flb_cf_destroy(cf);
    flb_cf_destroy(new_cf);
}


TEST_LIST = {
    { "reconstruct_cf"      , test_reconstruct_cf},
    { "reconstruct_cf_yaml" , test_reconstruct_cf_yaml},
    { 0 }
};