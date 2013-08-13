#include "calyx.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <json/json.h>

char MAIL[128], PASS[128];

void read_conf_file();
void parse_conf_line(char *line);
char *api_request(char *url, int r_meth, int s_meth, char *params);
size_t static write_callback_func(void *buffer, size_t size, size_t nmemb, void *userp);

struct list_item {
	char title[128];
	char id[128];
	int ep_seen;
	int ep_total;
	};
 
int main(void) {
	/* read config file */
	read_conf_file();

	/* authenticate */
	char *url = "https://hummingbirdv1.p.mashape.com/users/authenticate";
	char *auth_token = NULL;
	char param_buf[128];
	sprintf(param_buf, "email=%s&password=%s&", MAIL, PASS);
	/* the above magically appends "(null)" to the end of the string. therefore the closing "&" -- fuck strings in C */
	auth_token = api_request(url, 1, 0, param_buf);
	int atlen = strlen(auth_token) - 2;
	int i;
	char stripped_auth_token[atlen];
	for(i=1; i<atlen+1; i++) {
		stripped_auth_token[i-1] = auth_token[i];
		}
	stripped_auth_token[atlen] = '\0';

	/* get watching list */
	url = "https://hummingbirdv1.p.mashape.com/users/me/library";
	char *json_list = NULL;
	sprintf(param_buf, "id=me&status=currently-watching&auth_token=%s&", stripped_auth_token);
	api_request(url, 0, 1, param_buf);

	/* get file size */
	struct stat stat_buf;
	stat("list_cache.json", &stat_buf);
	long list_size = (long) stat_buf.st_size;

	/* read list file */
	FILE *list_fd;
	char list_line[list_size+1];
	if((list_fd = fopen("list_cache.json", "r")) == NULL) {
		perror("fopen");
		exit(1);
		}
	fgets(list_line, sizeof(list_line), list_fd);
	fclose(list_fd);

	/* parse json */
	json_object *jobj = json_tokener_parse(list_line);
	enum json_type type = json_object_get_type(jobj);
	if(type != json_type_array) {
		fprintf(stderr, "unexpected api reply\n");
		exit(1);
		}
	int list_len = json_object_array_length(jobj);
	json_object *list_object;
	json_object *anime_object;

	struct list_item anime_list[list_len];

	for (i=0; i<list_len; i++) {
		list_object = json_object_array_get_idx(jobj, i);
		anime_object = json_object_object_get(list_object, "anime");
		sprintf(anime_list[i].title, json_object_get_string(json_object_object_get(anime_object, "title")));
		sprintf(anime_list[i].id, json_object_get_string(json_object_object_get(anime_object, "slug")));
		anime_list[i].ep_seen = json_object_get_int(json_object_object_get(list_object, "episodes_watched"));
		anime_list[i].ep_total = json_object_get_int(json_object_object_get(anime_object, "episode_count"));
		}

	for (i=0; i<list_len; i++) {
		/*
		printf("%s [%d/%d] (%s)\n", anime_list[i].title,
			anime_list[i].ep_seen,
			anime_list[i].ep_total,
			anime_list[i].id);
		*/
		}

	return 0;
	}

void read_conf_file() {
	/* read config file */
	FILE *conf_fd;
	char conf_line[128];
	if((conf_fd = fopen("calyx.conf", "r")) == NULL) {
		perror("fopen");
		exit(1);
		}
	while(fgets(conf_line, sizeof(conf_line), conf_fd)) {
		/* parse config line */
		parse_conf_line(conf_line);
		}
	fclose(conf_fd);
	}

void parse_conf_line(char *line) {
	/* initialize stuff */
	int x, i;
	char name_buf[4];
	char val_buf[128];
	for(i=0; i<sizeof(val_buf); i++) {
		val_buf[i] = '\0';
		}
	regex_t patt;
	size_t nmatch = 3;
	regmatch_t pmatch[3];
	char patt_str[] = "^(mail|pass)=\"(.+)\"\n?$";
	/* apply regex pattern */
	if(regcomp(&patt, patt_str, REG_EXTENDED) != 0) {
		perror("regcomp");
		exit(1);
		}
	x = regexec(&patt, line, nmatch, pmatch, 0);
	if(!x) {
		snprintf(name_buf, (pmatch[1].rm_eo-pmatch[1].rm_so)+1, line+pmatch[1].rm_so);
		snprintf(val_buf, (pmatch[2].rm_eo-pmatch[2].rm_so)+1, line+pmatch[2].rm_so);
		if(strcmp(name_buf,"mail") == 0) {
			sprintf(MAIL, val_buf);
			}
		else if(strcmp(name_buf,"pass") == 0) {
			sprintf(PASS, val_buf);
			}
		}
	else {
		fprintf(stderr, "invalid config file\n");
		exit(1);
		}
	}

/* r_meth = request method (GET/POST), s_meth = store method (return/file) */
char *api_request(char *url, int r_meth, int s_meth, char *params) {
	CURL *curl_handle = NULL;
	char *response;
	char *header_part = "X-Mashape-Authorization: ";
	int i, n = (strlen(header_part) + strlen(API_KEY));
	char header[n];
	for(i=0; i<n; i++) {
		header[i] = '\0';
		}
	strcat(header, header_part);
	strcat(header, API_KEY);
	
	curl_handle = curl_easy_init();

	/* custom mashape header */
	struct curl_slist *chunk = NULL;
	chunk = curl_slist_append(chunk, header);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, chunk);

	if(r_meth) { /* POST */
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, params);
		}
	else { /* GET */
		char param_url[100];
		sprintf(param_url, url);
		strcat(param_url, "?");
		strcat(param_url, params);
		curl_easy_setopt(curl_handle, CURLOPT_URL, param_url);
		curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);
		}

	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);

	FILE *list_cache_fd;
	if(s_meth) { /* save to file  */
		response = "toast";
		if((list_cache_fd = fopen("list_cache.json", "w")) == NULL) {
			perror("fopen");
			exit(1);
			}
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, list_cache_fd);
		}
	else { /* return response */
		response = NULL;
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback_func); /* callback function */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response); /* pointer to callback parameter */
		}

	curl_easy_perform(curl_handle); /* perform the request */
	curl_easy_cleanup(curl_handle);
	//printf("strlen(response) in api_request: %d\n", strlen(response));
	if(s_meth) fclose(list_cache_fd);
	return response;
	}

/* the function to invoke as the data recieved */
size_t static write_callback_func(void *buffer, size_t size, size_t nmemb, void *userp) {
	char **response_ptr =  (char**)userp;
	/* assuming the response is a string */
	*response_ptr = strndup(buffer, (size_t)(size * nmemb));
	}
