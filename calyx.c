#include "calyx.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <curl/curl.h>

char MAIL[128], PASS[128];

void read_conf_file();
void parse_conf_line(char *line);
 
int main(void) {
	printf("api-key: %s", API_KEY);
	read_conf_file();
	printf("%s", MAIL);
	printf("%s", PASS);

	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "https://hummingbirdv1.p.mashape.com/users/authenticate");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			}
		/* always cleanup */ 
		curl_easy_cleanup(curl);
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
		if(strcmp(name_buf,"mail") != 0) {
			sprintf(MAIL, val_buf);
			}
		else if(strcmp(name_buf,"pass") != 0) {
			sprintf(PASS, val_buf);
			}
		}
	else {
		fprintf(stderr, "invalid config file\n");
		exit(1);
		}
	}
