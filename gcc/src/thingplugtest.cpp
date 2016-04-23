#include <iostream>
#include <pthread.h>
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include "curl/curl.h"
#include "json.h"
#include "mqtt/client.h"
/*
extern "C" {
    #include "MQTTClient.h"
}
*/

typedef struct thingplug_config {
    CURL *curl;

    char* nodeId;
    char* containerName;
    char* mgmtCmdPrefix;
    char* appId;
    char* uKey;
    char* passCode;
    char* cmdType;
    char* url;
    char* path;

    json_object* json;
    struct curl_slist* headers;

    char* nodeResourceId;
    char* dKey;
    char* lastContent;

    bool threadFlag;
    pthread_t p_thread;
    pthread_mutex_t mutex_lock;
} ThingplugConfig;

enum thingplug_step {
    device1,
    device2,
    device3,
    device4,
    device5,
    device6,
    application1,
    application2,
    application3,
};

static size_t response_callback(void* contents, size_t size, size_t nmemb, void* userp){
    ThingplugConfig* config = (ThingplugConfig*)userp;
    size_t realsize = size * nmemb;

    json_object* response_json = json_tokener_parse((char*)contents);
    
    json_object* nodeResourceId = NULL;
    json_object* contentInstance = NULL;

    //printf("content - %s\n", (char*)contents);
    json_object_object_get_ex(response_json, "ri", &nodeResourceId);
    json_object_object_get_ex(response_json, "con", &contentInstance);

    if (!config->nodeResourceId && nodeResourceId) {
	config->nodeResourceId = (char *)malloc(strlen(json_object_get_string(nodeResourceId)));
	snprintf(config->nodeResourceId, strlen(json_object_get_string(nodeResourceId)) + 1, 
                 "%s", json_object_get_string(nodeResourceId));
	json_object_put(nodeResourceId);
	nodeResourceId = NULL;
    }

    if (contentInstance) {
       if(config->lastContent){
           free(config->lastContent);
       }
       config->lastContent = (char *)malloc(strlen(json_object_get_string(contentInstance)));
       snprintf(config->lastContent, strlen(json_object_get_string(contentInstance)) + 1,
                 "%s", json_object_get_string(contentInstance));
       json_object_put(contentInstance);
       contentInstance = NULL; 
       printf("con[%s]", config->lastContent);
    }
	
    return realsize;
}

static size_t header_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    ThingplugConfig* config = (ThingplugConfig*)userp;
    size_t realsize = size * nmemb;

    //printf("headercallbck - size[%d] nmemb[%d] %s\n\n", size, nmemb, (char*)contents);
    char header_value[200];
	
    if (!config->dKey) {
        memset(header_value, 0x00, 200);
        sscanf((char*)contents, "dKey: %s", header_value);
        if (strlen(header_value) > 0) {
            config->dKey = (char *)malloc(strlen(header_value)+1);
            memset(config->dKey, 0x00, strlen(header_value+1));
            snprintf(config->dKey, strlen(header_value)+1, "%s", header_value);
        }
    }

    memset(header_value, 0x00, 200);
    sscanf((char*)contents, "Content-Location: %s", header_value);
    if (strlen(header_value) > 0) {
        fprintf(stdout, "content-location: %s\n", header_value);
    }
    return realsize;
}
static void pre_request(ThingplugConfig* config) {
    if (config->curl == NULL) {
        if ((config->curl = curl_easy_init()) == NULL) {
            /* log error */
            fprintf(stderr, "ERROR: Failed to create curl handle in fetch_session");
        }
    }
    else {
        fprintf(stderr, "ERROR: curl is not null ptr");
    }
}

static void post_request(thingplug_step step, ThingplugConfig* config) {
    printf("post request\n");
    long http_code = 0;
    curl_easy_getinfo(config->curl, CURLINFO_RESPONSE_CODE, &http_code);
    switch (step)
    {
        case device1:
            if(http_code == 409) fprintf(stdout, "이미 생성된 node resource ID 입니다.\n");
            fprintf(stdout, "node resource id : %s\n", config->nodeResourceId);
            break;
	case device2:
            if (http_code == 409) fprintf(stdout, "이미 생성된 remoteCSE 입니다.\n");
            fprintf(stdout, "디바이스 키[%d] : %s\n", strlen(config->dKey), config->dKey);
            break;
	case device3:
	    if (http_code == 409) fprintf(stdout, "이미 생성된 container 입니다.\n");
	    break;
	case device4:
	    if (http_code == 409) fprintf(stdout, "이미 생성된 mgmtCmd 입니다.\n");
	    break;
	case device5:
            if (http_code >= 400) fprintf(stdout, "sent content error\n");
            fprintf(stdout, "send data - content : %s\n", config->lastContent);
            break;
	case device6:
		break;
	case application1:
		break;
	case application2:
		break;
	case application3:
		break;
	default:
		break;
	}
	if (config->json) {
            json_object_put(config->json);
	    config->json = NULL;
	}
	if (config->headers) {
	    curl_slist_free_all(config->headers);
	    config->headers = NULL;
	}
	
	if(config->curl){
	    curl_easy_cleanup(config->curl);
	    config->curl = NULL;
	}
	
}

static struct curl_slist* append_header_value(struct curl_slist *headers, char* key, char* value) {
	char header_value[200];
	memset(header_value, 0x00, 200);
	snprintf(header_value, strlen(key) + strlen(value) + 1, "%s%s", key, value);
	//printf("set header end3 [%s][%d]\n", header_value, strlen(header_value));
	//headers = curl_slist_append(*headers, header_value);
	return curl_slist_append(headers, header_value);
	//return curl_slist_append(headers, "abcd");
}

#define APPEND_HEADER_VALUE(headers, key, value, header_value) \
memset(header_value, 0x00, 200); \
snprintf(header_value, strlen(key) + strlen(value) + 1, "%s%s", key, value); \
printf("set header end3 [%s][%d]\n", header_value, strlen(header_value)); \
headers = curl_slist_append(headers, header_value);\


static void set_headers(thingplug_step step, ThingplugConfig* config) {
	
        CURL* curl = config->curl;
	struct curl_slist *headers = NULL;
	
	char header_value[200];
	switch (step)
	{
	case device1:
		headers = append_header_value(headers, "X-M2M-Origin: ", config->nodeId);
		headers = append_header_value(headers, "X-M2M-RI: ", "123456");
		headers = append_header_value(headers, "X-M2M-NM: ", config->nodeId);
		headers = append_header_value(headers, "Accept: ", "application/json");
		headers = append_header_value(headers, "Content-Type: ", "application/json;ty=14");
		break;
#if 1
	case device2:
	        headers = append_header_value(headers, "X-M2M-Origin: ", config->nodeId);
		headers = append_header_value(headers, "X-M2M-RI: ", "123456");
		headers = append_header_value(headers, "X-M2M-NM: ", config->nodeId);
		headers = append_header_value(headers, "passCode: ", config->passCode);
		headers = append_header_value(headers, "Accept: ", "application/json");
		headers = append_header_value(headers, "Content-Type: ", "application/json;ty=16");
		break;
	case device3:
		headers = append_header_value(headers, "X-M2M-Origin: ", config->nodeId);
		headers = append_header_value(headers, "X-M2M-RI: ", "123456");
		headers = append_header_value(headers, "X-M2M-NM: ", config->containerName);
		headers = append_header_value(headers, "dkey: ", config->dKey);
		headers = append_header_value(headers, "locale: ", "ko");
		headers = append_header_value(headers, "Accept: ", "application/json");
		headers = append_header_value(headers, "Content-Type: ", "application/json;ty=3");
		break;
	case device4:
		headers = append_header_value(headers, "X-M2M-Origin: ", config->nodeId);
		headers = append_header_value(headers, "X-M2M-RI: ", "123456");
		
		memset(header_value, 0x00, 200);
		snprintf(header_value, strlen(config->mgmtCmdPrefix) + strlen(config->nodeId) + 1, "%s%s", config->mgmtCmdPrefix, config->nodeId);

		headers = append_header_value(headers, "X-M2M-NM: ", header_value);
		headers = append_header_value(headers, "dkey: ", config->dKey);
		headers = append_header_value(headers, "locale: ", "ko");
		headers = append_header_value(headers, "Accept: ", "application/json");
		headers = append_header_value(headers, "Content-Type: ", "application/json;ty=12");
		break;
#endif
	case device5:
                headers = append_header_value(headers, "X-M2M-Origin: ", config->nodeId);
                headers = append_header_value(headers, "X-M2M-RI: ", "123456");
                headers = append_header_value(headers, "dkey: ", config->dKey);
                headers = append_header_value(headers, "locale: ", "ko");
                headers = append_header_value(headers, "Accept: ", "application/json");
                headers = append_header_value(headers, "Content-Type: ", "application/json;ty=4");
		break;
	case device6:
		break;
	case application1:
		break;
	case application2:
		break;
	case application3:
		break;
	default:
		break;
	}
	if(headers){
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	}
}

static void set_contents(thingplug_step step, ThingplugConfig* config) {
        CURL* curl = config->curl;
	json_object *json = NULL;
	json = json_object_new_object();

	switch (step)
	{
	case device1:
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		json_object_object_add(json, "ni", json_object_new_string(config->nodeId));
		break;
	case device2: {
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		json_object_object_add(json, "cst", json_object_new_string("3"));
		json_object_object_add(json, "csi", json_object_new_string(config->nodeId));
		json_object * poa_array = json_object_new_array();
		char poastr[50];
		snprintf(poastr, strlen("MQTT|") + strlen(config->nodeId) + 1, "MQTT|%s", config->nodeId);
		json_object_array_add(poa_array, json_object_new_string(poastr));
		json_object_object_add(json, "poa", poa_array);
		json_object_object_add(json, "rr", json_object_new_string("true"));
		json_object_object_add(json, "nl", json_object_new_string(config->nodeResourceId));
		break;
	}
	case device3:
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		json_object_object_add(json, "containerType", json_object_new_string("heartbeat"));
		json_object_object_add(json, "heartbeatPeriod", json_object_new_string("300"));
		break;
	case device4:
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		json_object_object_add(json, "cmt", json_object_new_string(config->cmdType));
		json_object_object_add(json, "exe", json_object_new_string("true"));
		json_object_object_add(json, "ext", json_object_new_string(config->nodeResourceId));
		break;
	case device5:
                curl_easy_setopt(curl, CURLOPT_POST, 1);
                json_object_object_add(json, "cnf", json_object_new_string("text"));
                json_object_object_add(json, "con", json_object_new_string("123456"));
		break;
	case device6:
		break;
	case application1:
		break;
	case application2:
		break;
	case application3:
		break;
	default:
		break;
	}
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(json));
}

static void thingplug_request(thingplug_step step, ThingplugConfig* config) {
        CURL* curl = config->curl;
	CURLcode res;
	char full_url[200];
	memset(full_url, 0x00, 100);

	switch (step)
	{
	case device1:
		fprintf(stdout, "### 1. node 생성 ###\n");
		snprintf(full_url, strlen(config->url) + strlen("/ThingPlug") + 1, "%s/ThingPlug", config->url);
		break;
	case device2:
		fprintf(stdout, "### 2. remoteCSE 생성 ###\n");
		snprintf(full_url, strlen(config->url) + strlen("/ThingPlug") + 1, "%s/ThingPlug", config->url);
		break;
	case device3:
		fprintf(stdout, "### 3. container 생성 ###\n");
		snprintf(full_url, strlen(config->url) + strlen("/ThingPlug/remoteCSE-") + strlen(config->nodeId) + 1, "%s/ThingPlug/remoteCSE-%s", config->url, config->nodeId);
		break; 
	case device4:
		fprintf(stdout, "### 4. mgmtCmd 생성 ###\n");
		snprintf(full_url, strlen(config->url) + strlen("/ThingPlug") + 1, "%s/ThingPlug", config->url);
		break;
	case device5:
                fprintf(stdout, "### 6. 생성 ###\n");
                snprintf(full_url, strlen(config->url) + strlen("/ThingPlug/remoteCSE-") + strlen(config->nodeId) + strlen("/container-") + strlen(config->containerName) + 1, "%s/ThingPlug/remoteCSE-%s/container-%s", config->url, config->nodeId, config->containerName);

		break;
	case device6:
		break;
	case application1:
		break;
	case application2:
		break;
	case application3:
		break;
	default:
		break;
	}
	
	printf("URL[%s]\n", full_url);
	curl_easy_setopt(curl, CURLOPT_URL, full_url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, response_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)config);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)config);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
	res = curl_easy_perform(curl);
	printf("curl_easy_perform[%d]\n", res);
	if (res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
}

void* setContentInterval(void* arg) {
    ThingplugConfig* config = (ThingplugConfig*)arg;
    int tryCnt = 0;
    while (config->threadFlag && tryCnt < 10) {
        printf("SetContentInterval\n");
        tryCnt++;
        pthread_mutex_lock(&(config->mutex_lock));
        pre_request(config);
        set_headers(device5, config);
        set_contents(device5, config);
        thingplug_request(device5, config);
        post_request(device5, config);
        pthread_mutex_unlock(&(config->mutex_lock));

#if 0
		pre_request(&curl);
		set_headers(device5, curl, config);
		set_contents(device5, curl, config);
		thingplug_requeset(device5, curl, config);
		post_request(device5, &curl, config);
#endif
        usleep(1000*1000*3);
    }

}

void mqtt_listener(){

    mqtt::client client("tcp://sandbox.sktiot.com:1883", "clientId", NULL);
}

int main()
{
	curl_global_init(CURL_GLOBAL_ALL);

	ThingplugConfig* config = NULL;
	

	config = (ThingplugConfig*)malloc(sizeof(ThingplugConfig));
	memset(config, 0x00, sizeof(ThingplugConfig));
        config->curl = NULL;
	config->url = "http://sandbox.sktiot.com:9000";
	config->nodeId = "0.2.481.1.101.00000000000";
	config->passCode = "000101";
	config->appId = "myApplication";
	config->containerName = "myContainer";
	config->mgmtCmdPrefix = "myMGMT";
	config->cmdType = "sensor_1";
        pthread_mutex_init(&(config->mutex_lock), NULL);

	thingplug_step start_flags[5] = {device1, device2, device3, device4, device5};

    for (thingplug_step flag : start_flags) {
        switch (flag) {
            case device5: {
                config->threadFlag = true;
                int threadId = 0;
                threadId = pthread_create(&(config->p_thread), NULL, setContentInterval, (void *)config);
                if(threadId < 0){
                    printf("Make thread error\n");
                    config->threadFlag = false;
                }
                break;
            }
            default: {
                pthread_mutex_lock(&(config->mutex_lock));
                pre_request(config);
                set_headers(flag, config);
                set_contents(flag, config);
                thingplug_request(flag, config);
                post_request(flag, config);
                pthread_mutex_unlock(&(config->mutex_lock));
           }
        }
    }

    int status = 0;
    pthread_join(config->p_thread, (void **)&status);
    pthread_mutex_destroy(&(config->mutex_lock));
    curl_global_cleanup();
    return 0;
}

