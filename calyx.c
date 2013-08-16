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

struct packs {
	int ep_num;
	int pack_num;
	char line[512];
	};

struct packlist_ref {
	char hb_id[128];
	char packlist_title[128];
	char packlist_file_name[128];
	char subgroup[32];
	int latest_ep_num;
	int pak_count;
	struct packs paks[64];
	};

struct bot_info {
	char subgroup[32];
	char hb_title[128];
	int ep_seen;
	int pak_count;
	struct packs paks[64];
	};

void clean_screen();
void show_bot_info(struct bot_info, int mode);
int get_bot_watch_count();
void read_bot_watch_file(struct packlist_ref *p_refs);
void get_bot_packlists();
const char *strip_packlist_line(char *line);
const char *file_name_from_url(char *url);
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
	API_KEY = get_api_key();

	/* acquire xdcc packlists */
	int p_ref_count = get_bot_watch_count();
	struct packlist_ref p_refs[p_ref_count];
	read_bot_watch_file(p_refs);

	/* acquire hb anime list */
	read_conf_file();
	api_authenticate();
	api_get_json_list();
	struct list_item anime_buf[100];
	int list_len = get_c_list(anime_buf);
	struct list_item anime_list[list_len];
	int i;
	for(i=0; i<list_len; i++) {
		anime_list[i] = anime_buf[i];
		}

	/* get ncurses going */
	int rows, cols, title_len, info_len, j, k, t_idx, i_idx, curr_line,
		select_line=0, ep_diff, bot_inf;

	initscr();
	getmaxyx(stdscr, rows, cols);
	raw();
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
	char str_buf[cols+1], tmp_buf[cols], inp, update_url[128], update_params[128], info_buf[cols];
	int has_bot_info[rows];
	for(i=0; i<rows; i++) {
		has_bot_info[i] = 0;
		}
	struct bot_info curr_bot_info[rows];

	do {
		curr_line = 0;
		for(i=0; i<rows; i++) {
			if(i<list_len) {
				ep_diff = 0;
				bot_inf = 0;
				for(j=0; j<p_ref_count; j++) {
					if(strcmp(anime_list[i].id, p_refs[j].hb_id) == 0) {
						ep_diff = p_refs[j].latest_ep_num - anime_list[i].ep_seen;
						if(ep_diff) {
							bot_inf = 1;
							has_bot_info[i] = 1;
							sprintf(curr_bot_info[curr_line].subgroup, p_refs[j].subgroup);
							sprintf(curr_bot_info[curr_line].hb_title, anime_list[i].title);
							curr_bot_info[curr_line].pak_count = p_refs[j].pak_count;
							curr_bot_info[curr_line].ep_seen = anime_list[i].ep_seen;
							for(k=0; k<p_refs[j].pak_count; k++) {
								curr_bot_info[curr_line].paks[k].ep_num = p_refs[j].paks[k].ep_num;
								curr_bot_info[curr_line].paks[k].pack_num = p_refs[j].paks[k].pack_num;
								sprintf(curr_bot_info[curr_line].paks[k].line, p_refs[j].paks[k].line);
								}
							}
						}
					}
				if(bot_inf) sprintf(info_buf, "%s *%d", anime_list[i].title, ep_diff); /* unseen eps */
				else sprintf(info_buf, anime_list[i].title); /* no unseen eps */

				title_len = strlen(info_buf);
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
							str_buf[j] = info_buf[t_idx++];
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
				str_buf[cols] = '\0'; /* apparently strings in C can get larger than the char array itself?! therefore we cut off here*/
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
			case 's':
				if(has_bot_info[select_line]) {
					show_bot_info(curr_bot_info[select_line], 0);
					}
				break;
			case 'r':
				/* acquire new xdcc packlists */
				read_bot_watch_file(p_refs);
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

void clean_screen() {
	int i, j, rows, cols;
	getmaxyx(stdscr, rows, cols);
	char str_buf[cols];
	for(i=0; i<rows; i++) {
		for(j=0; j<cols; j++) {
			str_buf[j] = ' ';
			}
		mvprintw(i, 0, str_buf);
		}
	refresh();
	}

void show_bot_info(struct bot_info curr_bot_info, int mode) {
	clean_screen();
	int i, j, rows, cols, curr_row, curr_col;
	char inp;
	getmaxyx(stdscr, rows, cols);
	for(i=0; i<curr_bot_info.pak_count; i++) {
		if(curr_bot_info.paks[i].ep_num > curr_bot_info.ep_seen) attron(A_STANDOUT);
		if(mode) {
			mvprintw(i, 0, "%s", curr_bot_info.paks[i].line);
			}
		else {
			mvprintw(i, 0, "%d -> [%s] %s %d", curr_bot_info.paks[i].pack_num, curr_bot_info.subgroup,
				curr_bot_info.hb_title, curr_bot_info.paks[i].ep_num);
			}
		getyx(stdscr, curr_row, curr_col);
		for(j=curr_col; j<cols; j++) {
			mvprintw(i, j, " ");
			}
		if(curr_bot_info.paks[i].ep_num > curr_bot_info.ep_seen) attroff(A_STANDOUT);
		}
	refresh();
	inp = getch();
	clean_screen();
	if(inp == 'c') {
		show_bot_info(curr_bot_info, 1-mode);
		}
	}

int get_bot_watch_count() {
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
		fprintf(stderr, "unexpected bot watch syntax\n", bw_buf);
		exit(1);
		}
	return json_object_array_length(jobj);
	}

void read_bot_watch_file(struct packlist_ref *p_refs) {
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
		fprintf(stderr, "unexpected bot watch syntax\n", bw_buf);
		exit(1);
		}
	int list_len = json_object_array_length(jobj);
	json_object *bot_watch_object;

	int i, j, old, url_count=0;
	char url_tmp[128], urls[list_len][128];
	for(i=0; i<list_len; i++) {
		bot_watch_object = json_object_array_get_idx(jobj, i);
		sprintf(url_tmp, json_object_get_string(json_object_object_get(bot_watch_object, "url")));
		old=0; /* assume it's a new url */
		for(j=0; j<url_count; j++) {
			if(strcmp(url_tmp,urls[j]) == 0) { /* oh, it's not  */
				old=1;
				break;
				}
			}
		if(!old) {
			sprintf(urls[url_count++], url_tmp);
			}
		/* build array of packlist refs */
		sprintf(p_refs[i].hb_id, json_object_get_string(json_object_object_get(bot_watch_object, "hb_id")));
		sprintf(p_refs[i].packlist_title, json_object_get_string(json_object_object_get(bot_watch_object, "packlist_title")));
		sprintf(p_refs[i].subgroup, json_object_get_string(json_object_object_get(bot_watch_object, "group")));
		sprintf(p_refs[i].packlist_file_name, file_name_from_url(url_tmp));
		}

	for(i=0; i<url_count; i++) {
		/* download packlists */
		api_request(urls[i], 0, 1, "", (char*) file_name_from_url(urls[i]));
		}

	/* for each bot watch entry */
	for(i=0; i<list_len; i++) {
		int pak_count = 0;
		FILE *p_fd;
		p_fd = fopen(p_refs[i].packlist_file_name, "r");
		char p_line[512];
		int x, tmp_pak, tmp_ep;
		p_refs[i].latest_ep_num = 0;
		while(fgets(p_line, sizeof(p_line), p_fd)) { /* check all lines of the respective packlist */
			/* initialize regex stuff */
			regex_t patt;
			size_t nmatch = 3;
			regmatch_t pmatch[3];
			char patt_str[256], match_buf_pak[16], match_buf_ep[16];
			sprintf(patt_str, "^#([0-9]+).+?\\[%s].+?%s[^\\[\\(0-9]+?([0-9]+)", p_refs[i].subgroup, p_refs[i].packlist_title);
			/* apply regex pattern */
			if(regcomp(&patt, patt_str, REG_EXTENDED) != 0) {
				perror("regcomp");
				exit(1);
				}
			x = regexec(&patt, p_line, nmatch, pmatch, 0);
			if(!x) {
				pak_count++;
				snprintf(match_buf_pak, (pmatch[1].rm_eo-pmatch[1].rm_so)+1, p_line+pmatch[1].rm_so);
				tmp_pak = atoi(match_buf_pak);
				snprintf(match_buf_ep, (pmatch[2].rm_eo-pmatch[2].rm_so)+1, p_line+pmatch[2].rm_so);
				tmp_ep = atoi(match_buf_ep);
				if(tmp_ep > p_refs[i].latest_ep_num) p_refs[i].latest_ep_num = tmp_ep;
				p_refs[i].paks[pak_count-1].ep_num = tmp_ep;
				p_refs[i].paks[pak_count-1].pack_num = tmp_pak;
				sprintf(p_refs[i].paks[pak_count-1].line, strip_packlist_line(p_line));
				//printf("[%d][%d] %s\n", i, (pak_count-1), strip_packlist_line(p_line));
				}
			else {
				}
			regfree(&patt);
			}
		p_refs[i].pak_count = pak_count;
		fclose(p_fd);
		}
	}

const char *strip_packlist_line(char *line) {
	/* initialize stuff */
	int x, i;
	static char stripped_line[512] = "";
	regex_t patt;
	size_t nmatch = 2;
	regmatch_t pmatch[2];
	char patt_str[] = "(.+)\\s+$";
	/* apply regex pattern */
	if(regcomp(&patt, patt_str, REG_EXTENDED) != 0) {
		perror("regcomp");
		exit(1);
		}
	x = regexec(&patt, line, nmatch, pmatch, 0);
	if(!x) {
		snprintf(stripped_line, (pmatch[1].rm_eo-pmatch[1].rm_so)+1, line+pmatch[1].rm_so);
		}
	else {
		fprintf(stderr, "strip_packline_line failed\n");
		exit(1);
		}
	regfree(&patt);
	return stripped_line;
	}

const char *file_name_from_url(char *url) {
	/* initialize stuff */
	int x, i;
	static char file_name[128] = "";
	regex_t patt;
	size_t nmatch = 2;
	regmatch_t pmatch[2];
	char patt_str[] = "/([^/]+)$";
	/* apply regex pattern */
	if(regcomp(&patt, patt_str, REG_EXTENDED) != 0) {
		perror("regcomp");
		exit(1);
		}
	x = regexec(&patt, url, nmatch, pmatch, 0);
	if(!x) {
		snprintf(file_name, (pmatch[1].rm_eo-pmatch[1].rm_so)+1, url+pmatch[1].rm_so);
		}
	else {
		fprintf(stderr, "invalid url (needs a file name)\n");
		exit(1);
		}
	regfree(&patt);
	return file_name;
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
	regfree(&patt);
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
	int i, n = (strlen(header_part) + 32); /* 32 for API_KEY */
	char header[n];
	for(i=0; i<n; i++) {
		header[i] = '\0';
		}
	strcat(header, header_part);
	strncat(header, API_KEY, 32); /* API_KEY may contain random stuff after the 32nd character */
	
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
	if(s_meth) fclose(list_cache_fd);
	return response;
	}

/* the function to invoke as the data recieved */
size_t static write_callback_func(void *buffer, size_t size, size_t nmemb, void *userp) {
	char **response_ptr =  (char**)userp;
	/* assuming the response is a string */
	*response_ptr = strndup(buffer, (size_t)(size * nmemb));
	}
