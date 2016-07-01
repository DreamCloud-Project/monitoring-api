/*
 * Copyright 2016 High Performance Computing Center, Stuttgart
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "CuTest.h"
#include "mf_api.h"

void
Test_initialize(CuTest *tc)
{
    const char* server = "http://localhost:3030";
    const char* username = "test_user";
    const char* application = "myApp";
    const char* experiment_id = "uniqueId123";
    const char* job_id = "custom_id";

    const char* new_experiment_id = mf_api_new(
        server, username, application, experiment_id, job_id
    );
    CuAssertTrue(tc, strcmp(experiment_id, new_experiment_id) == 0);
}

void
Test_initialize_without_experiment_id(CuTest *tc)
{
    const char* server = "http://localhost:3030";
    const char* username = "test_user";
    const char* application = "myApp";
    const char* experiment_id = NULL;
    const char* job_id = "custom_id_2";

    const char* new_experiment_id = mf_api_new(
        server, username, application, experiment_id, job_id
    );
    CuAssertTrue(tc, strlen(new_experiment_id) == strlen("AVQzilzjcIVzfhf1PDL3"));
}

void
Test_initialize_without_job_id(CuTest *tc)
{
    const char* server = "http://localhost:3030";
    const char* username = "test_user";
    const char* application = "myApp";
    const char* experiment_id = NULL;
    const char* job_id = NULL;

    const char* new_experiment_id = mf_api_new(
        server, username, application, experiment_id, job_id
    );
    CuAssertTrue(tc, strlen(new_experiment_id) == strlen("AVQzilzjcIVzfhf1PDL3"));
}

void
Test_register_and_update(CuTest *tc)
{
    const char* server = "http://localhost:3030";
    const char* experiment_id = NULL;
    const char* username = "test_user";
    const char* application = "myApp";
    const char* job_id = "another custom id";
    mf_api_new(server, username, application, experiment_id, job_id);

    /* define metric */
    mf_metric* metric = malloc(sizeof(mf_metric));
    metric->timestamp = mf_api_get_time();
    metric->type = "foobar";
    metric->name = "progress (%)";
    metric->value = "20";

    char* response = mf_api_update(metric);
    CuAssertTrue(tc, strstr(response, "error") == NULL);
}

void
Test_register_and_update_multiple_times(CuTest *tc)
{
    const char* server = "http://localhost:3030";
    const char* experiment_id = NULL;
    const char* username = "test_user";
    const char* application = "myApp";
    const char* job_id = "yet another custom id";
    mf_api_new(server, username, application, experiment_id, job_id);
    char* response = malloc(sizeof(char) * 512);

    /* define metric */
    mf_metric* metric = malloc(sizeof(mf_metric));
    metric->type = "foobar";
    metric->name = "progress (%)";
    int value;
    for (value = 0; value != 50; ++value) {
        char tmp[3];
        sprintf(tmp, "%d", value * 2);
        metric->value = strdup(tmp);
        metric->timestamp = mf_api_get_time();

        memset(response, 0x00, 512);
        response = mf_api_update(metric);

        if (strstr(response, "error") != NULL) {
            return;
        }

        sleep(1);
    }

    CuAssertTrue(tc, strstr(response, "error") == NULL);
}

CuSuite* CuGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();

    // mf_api_new
    SUITE_ADD_TEST(suite, Test_initialize);
    SUITE_ADD_TEST(suite, Test_initialize_without_experiment_id);
    SUITE_ADD_TEST(suite, Test_initialize_without_job_id);

    // mf_api_update
    SUITE_ADD_TEST(suite, Test_register_and_update);
    SUITE_ADD_TEST(suite, Test_register_and_update_multiple_times);

    return suite;
}
