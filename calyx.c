#include "calyx.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <curl/curl.h>

char MAIL[128], PASS[128];

void read_conf_file();
void parse_conf_line(char *line);
char *api_request(char *url, int meth, char *params);
size_t static write_callback_func(void *buffer, size_t size, size_t nmemb, void *userp);
 
int main(void) {
	read_conf_file();
	printf("mail: %s\n", MAIL);
	printf("pass: %s\n", PASS);

	char *url = "https://hummingbirdv1.p.mashape.com/users/authenticate";
	//char *url = "https://hummingbirdv1.p.mashape.com/anime/steins-gate";
	char *content = NULL;
	char param_buf[128];
	sprintf(param_buf, "email=%s&password=%s&", MAIL, PASS); /* this magically appends "(null)" to the end of the string. therefor the appended & fuck strings in C */
	content = api_request(url, 1, param_buf);

	printf("%s", content);

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

char *api_request(char *url, int meth, char *params) {
	CURL *curl_handle = NULL;
	char *response = NULL;
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

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	if(meth) { /* POST */
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, params);
		}
	else { /* GET */
		curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);
		}

	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback_func); /* callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response); /* pointer to callback parameter */
	curl_easy_perform(curl_handle); /* perform the request */
	curl_easy_cleanup(curl_handle);
	return response;
	}

/* the function to invoke as the data recieved */
size_t static write_callback_func(void *buffer, size_t size, size_t nmemb, void *userp) {
	char **response_ptr =  (char**)userp;
	/* assuming the response is a string */
	*response_ptr = strndup(buffer, (size_t)(size *nmemb));
	}
