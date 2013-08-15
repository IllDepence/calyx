#include "calyx.h" 
#include <curl/curl.h>
#include <json/json.h>
#include <ncurses.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char MAIL[128], PASS[128], AUTH[32];
const char *API_KEY;

struct list_item {
	char title[128];
	char id[128];
	int ep_seen;
	int ep_total;
	};

void read_bot_watch_file();
void get_bot_packlists();
int num_len(int i);
int comp_ani(const void *v1, const void *v2);
void read_conf_file();
void parse_conf_line(char *line);
void api_authenticate();
void api_get_json_list();
int get_c_list(struct list_item *buf);
char *api_request(char *url, int r_meth, int s_meth, char *params, char *fname);
size_t static write_callback_func(void *buffer, size_t size, size_t nmemb, void *userp);
 
int main(void) {
	//api_request(url, 0, 1, param_buf, "");
	read_bot_watch_file();
	exit(0);
	API_KEY = get_api_key();
	read_conf_file();
	api_authenticate();
	api_get_json_list();
	struct list_item anime_buf[100];
	int list_len = get_c_list(anime_buf);
	int i;
	struct list_item anime_list[list_len];
	for(i=0; i<list_len; i++) {
		anime_list[i] = anime_buf[i];
		}

	/* get ncurses going */
	int rows, cols, title_len, info_len, j, t_idx, i_idx, curr_line, select_line=0;

	initscr();
	getmaxyx(stdscr, rows, cols);
	raw();
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
	char str_buf[cols], tmp_buf[cols], inp, update_url[128], update_params[128];

	do {
		curr_line = 0;
		for(i=0; i<rows; i++) {
			if(i<list_len) {
				title_len = strlen(anime_list[i].title);
				info_len = 3 + num_len(anime_list[i].ep_seen) + num_len(anime_list[i].ep_total);
				if(anime_list[i].ep_total != 0) {
					sprintf(tmp_buf, "[%d/%d]", anime_list[i].ep_seen, anime_list[i].ep_total);
					}
				else {
					sprintf(tmp_buf, "[%d/?]", anime_list[i].ep_seen);
					}

				t_idx = 0;
				i_idx = 0;
				for(j=0; j<cols; j++) {
					if(j < (cols-(3 + info_len))) {
						if(j < title_len) {
							/* insert title char */
							str_buf[j] = anime_list[i].title[t_idx++];
							}
						else {
							str_buf[j] = ' ';
							}
						}
					else {
						if(j < (cols-(info_len))) {
							str_buf[j] = ' ';
							}
						else {
							/* insert info char */
							str_buf[j] = tmp_buf[i_idx++];
							}
						}
					}
				if(curr_line == select_line) attron(A_STANDOUT);
				mvprintw(i, 0, str_buf);
				if(curr_line == select_line) attroff(A_STANDOUT);
				curr_line++;
				}
			else {
				for(j=0; j<cols; j++) {
					str_buf[j] = ' ';
					}
				}
			}

		refresh();
		inp = getch();
		int list_update = 0, delta;
		switch(inp) {
			case 'j':
				if(select_line < rows && select_line < list_len-1) select_line++;
				break;
			case 'k':
				if(select_line > 0) select_line--;
				break;
			case 'l':
				list_update = 1;
				delta = 1;
				break;
			case 'h':
				list_update = 1;
				delta = -1;
				break;
			default:
				break;
			}
		if(list_update) {
			/* send update api request */
			sprintf(update_url, "https://hummingbirdv1.p.mashape.com/libraries/%s", anime_list[select_line].id);
			sprintf(update_params, "anime_id=%s&auth_token=%s&episodes_watched=%d", anime_list[select_line].id,
				AUTH, (anime_list[select_line].ep_seen+delta));
			api_request(update_url, 1, 0, update_params, "");
			/* get updated list */
			api_get_json_list();
			list_len = get_c_list(anime_buf);
			for(i=0; i<list_len; i++) {
				anime_list[i] = anime_buf[i];
				}
			}
		getmaxyx(stdscr, rows, cols);
		} while(inp != 'q');

	endwin();

	return 0;
	}

void read_bot_watch_file() {
	/* get file size */
	struct stat stat_buf;
	stat("bot_watch.json", &stat_buf);
	long f_size = (long) stat_buf.st_size;

	/* read whole json file in bw_buf */
	FILE *bw_fd;
	char bw_buf[f_size];
	if((bw_fd = fopen("bot_watch.json", "r")) == NULL) {
		perror("fopen");
		exit(1);
		}
	fread(bw_buf, sizeof(char), f_size, bw_fd);
	fclose(bw_fd);

	/* parse json */
	json_object *jobj = json_tokener_parse(bw_buf);
	enum json_type type = json_object_get_type(jobj);
	if(type != json_type_array) {
		fprintf(stderr, "unexpected bot watch syntax\n");
		exit(1);
		}
	int list_len = json_object_array_length(jobj);
	json_object *bot_watch_object;

	int i, j, old, url_count=0;
	char url_tmp[128], urls[list_len][128];
	for(i=0; i<list_len; i++) {
		bot_watch_object = json_object_array_get_idx(jobj, i);
		sprintf(url_tmp, json_object_get_string(json_object_object_get(bot_watch_object, "url")));
		old=0; // assume it's a new url
		for(j=0; j<url_count; j++) {
			if(strcmp(url_tmp,urls[j]) == 0) { /* oh, it's not  */
				old=1;
				break;
				}
			}
		if(!old) {
			sprintf(urls[url_count++], url_tmp);
			}
		}

	for(i=0; i<url_count; i++) {
		printf("%s\n", urls[i]);
		}

	for(i=0; i<list_len; i++) {
		/*
		bot_watch_object = json_object_array_get_idx(jobj, i);
		printf("%s\n", json_object_get_string(json_object_object_get(bot_watch_object, "packlist_title")));
		list_object = json_object_array_get_idx(jobj, i);
		anime_object = json_object_object_get(list_object, "anime");
		sprintf(anime_list[i].title, json_object_get_string(json_object_object_get(anime_object, "title")));
		sprintf(anime_list[i].id, json_object_get_string(json_object_object_get(anime_object, "slug")));
		anime_list[i].ep_seen = json_object_get_int(json_object_object_get(list_object, "episodes_watched"));
		anime_list[i].ep_total = json_object_get_int(json_object_object_get(anime_object, "episode_count"));
		*/
		}
	}

int num_len(int i) {
	char c[10];
	sprintf(c, "%d", i);
	return strlen(c);
	}

int comp_ani(const void *v1, const void *v2) {
	const struct list_item *li1 = v1;
	const struct list_item *li2 = v2;
	return strcmp(li1->title, li2->title);
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

void api_authenticate() {
	char *url = "https://hummingbirdv1.p.mashape.com/users/authenticate";
	char *auth_token = NULL;
	char param_buf[128];
	sprintf(param_buf, "email=%s&password=%s&", MAIL, PASS);
	/* the above magically appends "(null)" to the end of the string. therefore the closing "&" -- fuck strings in C */
	auth_token = api_request(url, 1, 0, param_buf, "");
	int atlen = strlen(auth_token) - 2;
	int i;
	char stripped_auth_token[atlen];
	for(i=1; i<atlen+1; i++) {
		stripped_auth_token[i-1] = auth_token[i];
		}
	stripped_auth_token[atlen] = '\0';
	sprintf(AUTH, stripped_auth_token);
	}

void api_get_json_list() {
	/* get watching list */
	char *url = "https://hummingbirdv1.p.mashape.com/users/me/library";
	char *json_list = NULL;
	char param_buf[128];
	sprintf(param_buf, "id=me&status=currently-watching&auth_token=%s&", AUTH);
	api_request(url, 0, 1, param_buf, "list_cache.json");
	}

int get_c_list(struct list_item *anime_list) {
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

	int i;
	for(i=0; i<list_len; i++) {
		list_object = json_object_array_get_idx(jobj, i);
		anime_object = json_object_object_get(list_object, "anime");
		sprintf(anime_list[i].title, json_object_get_string(json_object_object_get(anime_object, "title")));
		sprintf(anime_list[i].id, json_object_get_string(json_object_object_get(anime_object, "slug")));
		anime_list[i].ep_seen = json_object_get_int(json_object_object_get(list_object, "episodes_watched"));
		anime_list[i].ep_total = json_object_get_int(json_object_object_get(anime_object, "episode_count"));
		}

	qsort(anime_list, list_len, sizeof(anime_list[0]), comp_ani);
	return list_len;
	}

/* r_meth = request method (GET/POST), s_meth = store method (return/file) */
char *api_request(char *url, int r_meth, int s_meth, char *params, char *fname) {
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
		if((list_cache_fd = fopen(fname, "w")) == NULL) {
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
