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
#include "mf_api.h"
#include "contrib/mf_debug.h"
#include "contrib/mf_publisher.h"

#include <ctype.h>    /* tolower */
#include <stdio.h>    /* snprintf */
#include <stdlib.h>   /* malloc */
#include <string.h>   /* memcpy, strlen */
#include <netdb.h>    /* freeaddrinfo */
#include <sys/time.h> /* gettimeofday */
#include <time.h>     /* strftime, localtime */
#include <unistd.h>   /* gethostname */
#include <math.h>     /* floor */

/*******************************************************************************
 * Variable Declarations
 ******************************************************************************/

typedef struct mf_state_t mf_state;

struct mf_state_t {
    const char* experiment_id;
    const char* user;
    const char* application;
    const char* hostname;
    const char* server;
    const char* job_id;
    const char* path;
    const char* date;
} mf_state_t;

mf_state* status;

/*******************************************************************************
 * Forward Declarations
 ******************************************************************************/

static void to_lowercase(char* word, int length);
static void get_hostname(char* hostname);
char* mf_api_get_time();
void convert_time_to_char(double ts, char* time_stamp);

/*******************************************************************************
 * mf_api_new
 ******************************************************************************/

const char*
mf_api_new(
    const char* server,
    const char* user,
    const char* application,
    const char* experiment_id,
    const char* job_id)
{
    if (server == NULL || server[0] == '\0') {
        log_error("parameter 'server' is not set (%s)", server);
        return NULL;
    }
    if (user == NULL || user[0] == '\0') {
        log_error("parameter 'user' is not set (%s)", user);
        return NULL;
    }

    if (status == NULL) {
        status = malloc(sizeof(mf_state));
    }
    status->server = strdup(server);
    status->path = strdup("v1/mf/metrics");

    char tmp[strlen(user)];
    strcpy(tmp, user);
    to_lowercase(tmp, strlen(tmp));
    status->user = strdup(tmp);

    /* handle input parameter 'application' */
    if (application == NULL || application[0] == '\0') {
        status->application = strdup("_all");
    } else {
        strcpy(tmp, application);
        to_lowercase(tmp, strlen(tmp));
        status->application = strdup(tmp);
    }

    /* handle input parameter 'experiment_id' */
    if (experiment_id == NULL || experiment_id[0] == '\0') {
        status->experiment_id = NULL;
    } else {
        status->experiment_id = strdup(experiment_id);
    }

    /* handle input parameter 'job_id') */
    if (job_id == NULL || job_id[0] == '\0') {
        status->job_id = strdup("mf_api");
    } else {
        status->job_id = strdup(job_id);
    }

    char* hostname = malloc(sizeof(char) * 254);
    get_hostname(hostname);
    status->hostname = strdup(hostname);

    /*
     * either create a new experiment_id if uninitialized,
     * otherwise mf_create_user just returns the given id
     */
    char message[1000] = "";
    sprintf(message,
        "{ \
          \"host\":\"%s\", \
          \"@timestamp\":\"%s\", \
          \"user\":\"%s\", \
          \"application\":\"%s\", \
          \"job_id\":\"%s\" \
        }",
        status->hostname,
        mf_api_get_time(),
        status->user,
        status->application,
        status->job_id
    );

    const char* response = mf_create_user(
        status->server, status->user, status->experiment_id, message
    );

    /* we just expect that the reponse is correct */
    status->experiment_id = strdup(response);

    return response;
}

/*******************************************************************************
 * mf_api_get_server
 ******************************************************************************/

const char*
mf_api_get_server()
{
    return (status != NULL) ? status->server : NULL;
}

/*******************************************************************************
 * mf_api_get_id
 ******************************************************************************/

const char*
mf_api_get_id()
{
    return (status != NULL) ? status->experiment_id : NULL;
}

/*******************************************************************************
 * mf_api_get_user
 ******************************************************************************/

const char*
mf_api_get_user()
{
    return (status != NULL) ? status->user : NULL;
}

/*******************************************************************************
 * mf_api_get_application
 ******************************************************************************/

const char*
mf_api_get_application()
{
    return (status != NULL) ? status->application : NULL;
}

/*******************************************************************************
 * mf_api_get_job_id
 ******************************************************************************/

const char*
mf_api_get_job_id()
{
    return (status != NULL) ? status->job_id : NULL;
}

/*******************************************************************************
 * mf_api_update
 ******************************************************************************/

char*
mf_api_update(mf_metric* metric)
{
    char curl_data[4095] = { 0 };
    if (metric->timestamp == '\0') {
        metric->timestamp = strdup(mf_api_get_time());
    }

    sprintf(curl_data,
        "{ \
            \"@timestamp\": \"%s\", \
            \"host\":\"%s\", \
            \"task\":\"%s\", \
            \"type\": \"%s\", \
            \"%s\": \"%s\" \
        }",
        metric->timestamp,
        status->hostname,
        status->application,
        metric->type,
        metric->name,
        metric->value
    );

    char URL[256];
    sprintf(URL, "%s/%s/%s/%s?task=%s",
        status->server,
        status->path,
        status->user,
        status->experiment_id,
        status->application
    );

    return publish_json(URL, curl_data);
}

/*******************************************************************************
 * mf_api_clear
 ******************************************************************************/

void
mf_api_clear()
{
    memset(&status, 0, sizeof(status));
}

/*******************************************************************************
 * to_lowercase
 ******************************************************************************/

static void
to_lowercase(char* word, int length)
{
    int i;
    for(i = 0; i < length; i++) {
        word[i] = tolower(word[i]);
    }
}

/*******************************************************************************
 * get_fully_qualified_domain_name
 ******************************************************************************/

static int
get_fully_qualified_domain_name(char *fqdn)
{
    struct addrinfo hints, *info, *p;

    int gai_result;

    char *hostname = malloc(sizeof(char) * 80);
    gethostname(hostname, sizeof hostname);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    if ((gai_result = getaddrinfo(hostname, "http", &hints, &info)) != 1) {
        FILE *tmp = NULL;
        if ((tmp = popen("hostname", "r")) == NULL ) {
            perror("popen");
            return -1;
        }
        char line[200];
        while (fgets(line, 200, tmp) != NULL )
            sprintf(fqdn, "%s", line);
        return 1;
    }
    for (p = info; p != NULL ; p = p->ai_next) {
        sprintf(fqdn, "%s\n", p->ai_canonname);
    }

    if (info->ai_canonname)
        freeaddrinfo(info);

    return 1;
}

/*******************************************************************************
 * get_hostname
 ******************************************************************************/

void
get_hostname(char* hostname)
{
    get_fully_qualified_domain_name(hostname);
    hostname[strlen(hostname) - 1] = '\0';
}

/*******************************************************************************
 * get_time_as_string
 ******************************************************************************/

static void
get_time_as_string(char* timestamp, const char* format, int in_milliseconds)
{
    char fmt[64];
    char buf[64];
    struct timeval tv;
    time_t current_time;
    struct tm *tm;
    int cut_of = 0;
    if (in_milliseconds) {
        cut_of = 3;
    }

    gettimeofday(&tv, NULL);
    current_time = tv.tv_sec;

    /* get timestamp */
    if((tm = localtime(&current_time)) != NULL) {
        // yyyy-MM-dd’T'HH:mm:ss.SSS
        strftime(fmt, sizeof fmt, "%Y-%m-%dT%H:%M:%S.%%6u", tm);
        snprintf(buf, sizeof buf, fmt, tv.tv_usec);
    }

    memcpy(timestamp, buf, strlen(buf) - cut_of);
    timestamp[strlen(buf) - cut_of] = '\0';

    /* replace whitespaces in timestamp: yyyy-MM-dd’T'HH:mm:ss. SS */
    int i = 0;
    while (timestamp[i++]) {
        if (isspace(timestamp[i])) {
            timestamp[i] = '0';
        }
    }
}

/*******************************************************************************
 * mf_api_get_time
 ******************************************************************************/

char*
mf_api_get_time()
{
    char* timestamp = malloc(sizeof(char) * 64);
    get_time_as_string(timestamp, "%Y-%m-%dT%H:%M:%S.%%6u", 1);
    debug("TIMESTAMP: %s", timestamp);
    return timestamp;
}

