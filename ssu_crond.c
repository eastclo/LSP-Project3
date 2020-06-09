#include "default.h"
#include "ssu_crond.h"

FILE *file_fp; //ssu_crontab_file
FILE *log_fp; //ssu_crontab_log
char crontabFile[FILELEN]; //ssu_crontab_file 경로
char logFile[FILELEN]; //ssu_crontab_log 경로

int main(void)
{
	struct tm nowTime; 
	time_t now;
	char buf[FILELEN];
	int fd;

	//디몬으로 실행 전 log파일과 ssu_crontab_file의 경로 저장
	getcwd(buf, FILELEN);
	sprintf(crontabFile, "%s/ssu_crontab_file", buf);
	sprintf(logFile, "%s/ssu_crontab_log", buf);	

	//ssu_crontab_file이 없을 시 생성
	if(access(crontabFile, F_OK) < 0) {
		if((fd = creat(crontabFile, 0666)) < 0) {
			fprintf(stderr, "creat error for %s\n", crontabFile);
			exit(1);
		}
		close(fd);
	}
	//ssu_crontab_log가 없을 시 생성
	if(access(logFile, F_OK) < 0) {
		if((fd = creat(logFile, 0666)) < 0) {
			fprintf(stderr, "creat error for %s\n", logFile);
			exit(1);
		}
		close(fd);
	}

	//디몬 프로세스로 실행
//	if(daemon_init() < 0) {
//		fprintf(stderr, "daemon process isn't created\n");
//		exit(1);
//	}

	chdir(buf);//현재 작업디렉토리로 이동
	
	//시간을 x시 x분 00초에 맞춤
	do {
		sleep(1);
		now = time(NULL);
		localtime_r(&now, &nowTime);
	} while(nowTime.tm_sec != 0);

	while (1) {
		//매 분마다 명령어 실행
		execute_crontab_cmd(now);
		sleep(59);
		do {
			now = time(NULL);
			localtime_r(&now, &nowTime);
		} while(nowTime.tm_sec != 0);
	}

	exit(0);
}

int daemon_init(void) //디몬프로세스 생성
{
	pid_t pid;
	int fd, maxfd;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}

	else if (pid != 0) //1. 백그라운드 실행
		exit(0);

	setsid(); //2. 새 세션생성
	signal(SIGTTIN, SIG_IGN); //3. 입출력 시그널 무시
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();

	for (fd = 0; fd < maxfd; fd++) //6. 오픈된 모든 fd닫기
		close(fd);

	umask(0); //4. 파일모드 생성 마스크 해제
	chdir("/"); //5. 현재 디렉토리를 루트 디렉토리로 설정
	fd = open("/dev/null", O_RDWR); //7. 표준 입출력과 표준에러를 /dev/null로 재지정
	dup(0);
	dup(0);
	return 0;
}

void execute_crontab_cmd(time_t now) //ssu_crontab_file에서 now시간에 실행할 명령어 실행
{
	struct flock lock; //crontab과 동시에 쓰기 방지를 위한 락 변수
	char buf[BUFLEN];
	int fd, i;
	
	file_fp = fopen(crontabFile, "r");
	fd = fileno(file_fp);
	
	lock.l_type = F_RDLCK;
	lock.l_whence = 0;
	lock.l_start = 0L;
	lock.l_len = 0L;

	//파일 락 설정, crontab에서 쓰고 있으면 기다림
	while(fcntl(fd, F_SETLK, &lock) == -1);

	//파일 끝에 도달할 때까지 라인단위로 읽음
	while(fscanf(file_fp, " %[^\n]", buf) != EOF) {
		if(check_times(now, buf)) { //실행할 시간이라면
			execute_cmd(buf);
			write_run_log(now, buf); //now시간, run msg 로그 기록
		}
	}

	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	fclose(file_fp);
}

int check_times(time_t now, char *cmd) //cmd가 실행할 시간이면 true, 아니면 false
{
	struct tm nowTime;
	char buf[BUFLEN];
	char *min, *hour, *day, *month, *week;

	strcpy(buf, cmd);
	min = strtok(buf, " ");
	hour = strtok(NULL, " ");
	day = strtok(NULL, " ");
	month = strtok(NULL, " ");
	week = strtok(NULL, " ");

	localtime_r(&now, &nowTime);
	//각 항목 중 하나라도 해당하지 않으면 false
	if(!check_time(nowTime.tm_wday, WEEK, week))
		return false;
	if(!check_time(nowTime.tm_mon, MONTH, month))
		return false;
	if(!check_time(nowTime.tm_mday, DAY, day))
		return false;
	if(!check_time(nowTime.tm_hour, HOUR, hour))
		return false;
	if(!check_time(nowTime.tm_min, MIN, min))
		return false;
	
	return true;
}

int check_time(int time_val, int flag, char *cycle) //time_val이 flag(분,시,일,월,주)에 따라 cycle에 해당하면 true
{
	char check[60] = {0,};
	char lexeme[10] = {0,};
	int lo, hi, i, j, lexlen = 0;
	int low, high;
	switch (flag) {
		case MIN :
			lo = 0, hi = 59;
		break;
		case HOUR :
			lo = 0, hi = 23;
		break;
		case DAY :
			lo = 1, hi = 31;
		break;
		case MONTH :
			lo = 1, hi = 12;
			time_val++;	//tm구조체의 tm_mon은 0~11이므로
		break;
		case WEEK :
			lo = 0, hi = 6;
		break;
	}

	for(i = 0; i < strlen(cycle); i++) {
		lexlen = 0;
		//숫자 일 때
		if(isdigit(cycle[i])) { 
			while(isdigit(cycle[i])) 
				lexeme[lexlen++] = cycle[i++];
			lexeme[lexlen] = 0;

			if(cycle[i] != '-') { //범위가 아니라면 해당 숫자만 체크
				i--;
				check[atoi(lexeme)] = true;
			}
			else { //범위라면 해당 범위 전체 체크
				low = atoi(lexeme);			
				i++, lexlen = 0;
				while(isdigit(cycle[i])) 
					lexeme[lexlen++] = cycle[i++];
				lexeme[lexlen] = 0;
				high = atoi(lexeme);

				//뒤에 '/'일 경우
				if(cycle[i] == '/') {
					i++,lexlen = 0;
					while(isdigit(cycle[i])) 
						lexeme[lexlen++] = cycle[i++];
					lexeme[lexlen] = 0;
					i--;

					for(j = 0; j + low <= high; j++) {
						if((j+1)%atoi(lexeme) == 0)
							check[j+low] = true;
					}
				}
				else {
					i--;
					for(j = low; j <= high; j++) 
						check[j] = true;
				}
			}
		}
		//'*'은 범위 전체 체크
		else if(cycle[i] == '*') {
			if(i+1 < strlen(cycle)) {
				if(cycle[i+1] != '/') {
					for(j = lo; j <= hi; j++) 
						check[j] = true;
					break;
				}
				//뒤에 '/'일경우 추가
				else {
					i += 2;
             		while(isdigit(cycle[i]))
                		 lexeme[lexlen++] = cycle[i++];
        		    lexeme[lexlen] = 0;
		            i--;
					for(j = 0; j + lo <= hi; j++) {
						if((j+1)%atoi(lexeme) == 0)
							check[j+lo] = true;
					}
				}
			}
			else {
				for(j = lo; j <= hi; j++) 
					check[j] = true;
					break;
			}
		}
		//','는 병렬체크
		else if(cycle[i] == ',') 
			low = 0, high = 0;
	}

	if(check[time_val] == false)
		return false;
	else
		return true;
}

void execute_cmd(char *buf) //buf에서 cmd부분 실행
{
	char tmp[BUFLEN];
	char *ptr;	
	strcpy(tmp, buf);

	ptr = strtok(tmp, " "); //min
	ptr = strtok(NULL, " "); //hour
	ptr = strtok(NULL, " "); //day
	ptr = strtok(NULL, " "); //month
	ptr = strtok(NULL, " "); //week
	ptr = strtok(NULL, " "); //cmd

	system(buf+(ptr-tmp)); //strtok로 인해 공백에 NULL이 들어가므로 buf에서 출력한다
}

void write_run_log(time_t now, char *msg) //now시간 run msg를 로그에 기록
{
	char* time_ptr; 
	struct tm nowTime;
	struct flock lock; //crontab과 동시에 쓰기 방지를 위한 락 변수	
	int fd;

	log_fp = fopen(logFile, "r+");
	fd = fileno(log_fp);

	localtime_r(&now, &nowTime);

	lock.l_type = F_WRLCK;
	lock.l_whence = 0;
	lock.l_start = 0L;
	lock.l_len = 0L;

	//파일 락 설정, crontab에서 쓰고 있으면 기다림
	while(fcntl(fd, F_SETLK, &lock) == -1);

	fseek(log_fp, 0, SEEK_END);
	//시간 기록 [Mon May 11 11:57:23 2020] run 명령어 기록
	time_ptr = ctime(&now);
	time_ptr[strlen(time_ptr)-1] = ']';
	fprintf(log_fp, "[%s run %s\n", time_ptr, msg);

	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	fclose(log_fp);
}
