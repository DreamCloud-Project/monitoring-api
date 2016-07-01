/*
 * Copyright 2014, 2015 High Performance Computing Center, Stuttgart
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

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>


#include "mf_debug.h"
#include "mf_publisher.h"

static CURL *curl;
char execution_id[ID_SIZE] = { 0 };
struct curl_slist *headers = NULL;

static void
init_curl()
{
    if (curl != NULL ) {
        return;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (headers != NULL ) {
        return;
    }

    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charsets: utf-8");
}

struct string {
    char *ptr;
    size_t len;
};

void
init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

static size_t
get_stream_data(void *buffer, size_t size, size_t nmemb, void *stream) {
    size_t total = size * nmemb;
    memcpy(stream, buffer, total);

    return total;
}

static int
check_URL(const char *URL)
{
    if (URL == NULL || *URL == '\0') {
        const char *error_msg = "URL not set.";
        log_error("publish(const char*, Message) %s", error_msg);
        return 0;
    }
    return 1;
}

static int
check_message(const char *message)
{
    if (message == NULL || *message == '\0') {
        const char *error_msg = "message not set.";
        log_error("publish(const char*, Message) %s", error_msg);
        return 0;
    }
    return 1;
}

static int
prepare_publish(const char *URL, const char *message)
{
    init_curl();

    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long ) strlen(message));
    #ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    #endif

    return 1;
}

static int
prepare_query(const char* URL)
{
    init_curl();

    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    #ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    #endif

    return 1;
}

size_t
curl_write( void *ptr, size_t size, size_t nmemb, void *stream)
{
    return fwrite(ptr, size, nmemb, stdout);
}

int
query(const char* query, char* received_data)
{
    int result = SEND_SUCCESS;
    struct string response_message;

    if (!check_URL(query)) {
        return 0;
    }

    if (!prepare_query(query)) {
        return 0;
    }

    init_string(&response_message);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_message);

    CURLcode response = curl_easy_perform(curl);
    if (response != CURLE_OK) {
        result = SEND_FAILED;
        const char *error_msg = curl_easy_strerror(response);
        log_error("query(const char*, char*) %s", error_msg);
    }

    received_data = (char*) realloc (received_data, response_message.len);
    received_data = response_message.ptr;

    curl_easy_reset(curl);

    return result;
}

char*
publish_json(const char *URL, const char *message)
{
    if (!check_URL(URL) || !check_message(message)) {
        return 0;
    }

    if (!prepare_publish(URL, message)) {
        return 0;
    }

    char* response_message = malloc(sizeof(char) * 1024);
    memset(response_message, 0x00, 1024);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_stream_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_message);

    CURLcode response = curl_easy_perform(curl);
    if (response != CURLE_OK) {
        const char *error_msg = curl_easy_strerror(response);
        log_error("publish(const char*, Message) %s", error_msg);
    }

    debug("URL %s + RESPONSE: %s", URL, response_message);
    curl_easy_reset(curl);

    return response_message;
}

char*
get_execution_id(const char *URL, char *message)
{
    if (strlen(execution_id) > 0) {
        return execution_id;
    }

    if (!check_URL(URL) || !check_message(message)) {
        return '\0';
    }

    if (!prepare_publish(URL, message)) {
        return '\0';
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_stream_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &execution_id);

    CURLcode response = curl_easy_perform(curl);
    if (response != CURLE_OK) {
        const char *error_msg = curl_easy_strerror(response);
        log_error("publish(const char*, Message) %s", error_msg);
    }

    debug("get_execution_id(const char*, char*) Execution_ID = <%s>", execution_id);

    curl_easy_reset(curl);

    return execution_id;
}

void
shutdown_curl()
{
    if (curl == NULL ) {
        return;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

int
mf_head(const char* URL)
{
    init_curl();

    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);

    CURLcode response = curl_easy_perform(curl);
    curl_easy_reset(curl);

    if (response != CURLE_OK) {
        const char *error_msg = curl_easy_strerror(response);
        log_error("mf_head(const char*) %s", error_msg);
        return 0;
    }

    return 1;
}

char*
mf_register_workflow(
    const char* URL,
    const char* workflow,
    const char* json_string)
{
    init_curl();
    char* response_message = malloc(sizeof(char) * 1024);
    memset(response_message, 0x00, 1024);

    const char* index = "v1/mf/users";
    char* newURL = malloc(sizeof(char) * (strlen(URL) + strlen(index) + strlen(workflow) + 4));
    sprintf(newURL, "%s/%s/%s", URL, index, workflow);

    curl_easy_setopt(curl, CURLOPT_URL, newURL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_stream_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_message);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);

    CURLcode response = curl_easy_perform(curl);
    if (response != CURLE_OK) {
        const char *error_msg = curl_easy_strerror(response);
        log_error("mf_register_workflow(const char*) %s", error_msg);
    } else {
        debug("RESPONSE: %s", response_message);
    }
    curl_easy_reset(curl);
    free(newURL);

    return response_message;
}

char* mf_create_user(
  const char* server,
  const char* username,
  const char* experiment_id,
  const char* message)
{
    init_curl();

    const char* resource = "v1/mf/users";
    char* URL;

    if (experiment_id == NULL || experiment_id == '\0') {
        URL = malloc(
            sizeof(char) *
            (strlen(server) +
            strlen(resource) +
            strlen(username) +
            10)
        );
        sprintf(URL, "%s/%s/%s/create", server, resource, username);
    } else {
        URL = malloc(
            sizeof(char) *
            (strlen(server) +
             strlen(resource) +
             strlen(username) +
             strlen(experiment_id) +
            12)
        );
        sprintf(URL, "%s/%s/%s/%s/create", server, resource, username, experiment_id);
    }

    char* json = malloc(sizeof(char) * 128 + strlen(username));

    /* include message as body */
    if (message == NULL || message[0] == '\0') {
        sprintf(json, "{ \"user\": \"%s\" }", username);
    } else {
        json = strdup(message);
    }

    return publish_json(URL, json);
}

char*
mf_create_experiment(
    const char* server,
    const char* workflow,
    const char* json_string)
{
    char* resource = malloc(sizeof(char) * 128);
    const char* index = "v1/dreamcloud/mf/experiments";

    sprintf(resource, "%s/%s", index, workflow);
    char* URL = malloc(sizeof(char) *
            (strlen(server) + strlen(index) + strlen(workflow) + 4));
    sprintf(URL, "%s/%s", server, resource);

    debug("CREATE(URL): %s", URL);

    return publish_json(URL, json_string);
}
