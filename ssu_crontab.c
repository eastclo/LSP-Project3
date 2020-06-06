#include "default.h"
#include "ssu_crontab.h"

FILE *file_fp; //ssu_crontab_file
FILE *log_fp; //ssu_crontab_log
char crontabFile[FILELEN]; //ssu_crontab_file 경로
char logFile[FILELEN]; //ssu_crontab_log 경로
int cmd_count; //ssu_crontab_file의 명령어 개수

int main(void)
{
    char buf[BUFLEN];
	char command[BUFLEN];
	int fd;

	//log파일과 ssu_crontab_file의 경로 저장
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

	while (1) {
		memset(command, 0, BUFLEN);
		print_prompt();
		fgets(command, sizeof(command), stdin);

		if(command[0] == '\n')
			continue;
		else
			command[strlen(command)-1] = 0;

		if(execute_command(command) < 0)
			break;
	}

	exit(0);
}

void print_prompt()
{
    struct flock lock; //crontab과 동시에 쓰기 방지를 위한 락 변수
    char buf[BUFLEN];
	char *prompt = "20162444>";
    int fd;
	
	cmd_count = 0; //초기화

    file_fp = fopen(crontabFile, "r");
    fd = fileno(file_fp);

    lock.l_type = F_RDLCK;
    lock.l_whence = 0;
    lock.l_start = 0L;
    lock.l_len = 0L;

    //파일 락 설정, crontab에서 쓰고 있으면 기다림
    while(fcntl(fd, F_SETLK, &lock) == -1);

    //파일 끝에 도달할 때까지 라인단위로 읽음
    while(fscanf(file_fp, " %[^\n]", buf) != EOF) 
		fprintf(stdout, "%d. %s\n", cmd_count++, buf);

    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(file_fp);

	printf("\n");

	fputs(prompt, stdout);
}

int execute_command(char *command) //명령어 구분하여 각각의 명령어 실행함수 호출
{
	char *ptr;
	if (strcmp(command, "exit") == 0) //프로그램 종료
		return -1;

	ptr = strtok(command, " ");
	if(strcmp(ptr, "add") == 0) { //추가 명령어: add <실행주기> <명령어>
		ptr = ptr + strlen(ptr) + 1;
		cmd_add(ptr);
	}
	else if(strcmp(ptr, "remove") == 0) { //삭제 명령어: remove <COMMAND_NUMBER>
		ptr = ptr + strlen(ptr) + 1;
		cmd_remove(ptr);
	}
	else
		print_help();

	return 1;
}

void cmd_add(char *cmd) //add명령어 실행
{
	struct flock lock; //crontab과 동시에 쓰기 방지를 위한 락 변수
	int fd;

	if(check_add_cmd(cmd) == false) //인자 검사 오류시 false
		return;

	file_fp = fopen(crontabFile, "r+");
	fd = fileno(file_fp);

	lock.l_type = F_WRLCK;
	lock.l_whence = 0;
	lock.l_start = 0L;
	lock.l_len = 0L;

	//파일 락 설정, crontab에서 쓰고 있으면 기다림
	while(fcntl(fd, F_SETLK, &lock) == -1);

	fseek(file_fp, 0, SEEK_END);
	//crontab파일에 기록
	fprintf(file_fp, "%s\n", cmd);

	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	fclose(file_fp);

	//add 로그 기록
	write_log(cmd, "add");
}

int check_add_cmd(char *cmd) //TODO:실행주기 오류시 false리턴
{
	return true;
}

void write_log(char *msg, char *flag) //현재 시간, flag(run, add) msg를 로그에 기록
{
	struct tm nowTime;
	struct flock lock; //crontab과 동시에 쓰기 방지를 위한 락 변수
	char* time_ptr;
	time_t now;
	int fd;

	log_fp = fopen(logFile, "r+");
	fd = fileno(log_fp);

	now = time(NULL);
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
	fprintf(log_fp, "[%s %s %s\n", time_ptr, flag, msg);

	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	fclose(log_fp);
}
  
void cmd_remove(char *cmd) //remove 명령어 실행
{
	struct flock lock; //crontab과 동시에 쓰기 방지를 위한 락 변수
	char buf[BUFLEN];
	char remove_line[BUFLEN];
	int fd, i = 0;
	FILE *ftmp;

	if(check_remove_cmd(cmd) == false) //인자 검사 오류시 false
		return;

	file_fp = fopen(crontabFile, "r+");
	fd = fileno(file_fp);

	lock.l_type = F_WRLCK;
	lock.l_whence = 0;
	lock.l_start = 0L;
	lock.l_len = 0L;

	//파일 락 설정, crontab에서 쓰고 있으면 기다림
	while(fcntl(fd, F_SETLK, &lock) == -1);

 	//지우려는 라인전 까지 읽음
	for(i = 0; i < atoi(cmd); i++)
    	fscanf(file_fp, " %[^\n]", buf);

	ftmp = fopen(crontabFile, "r+");
	fseek(ftmp , ftell(file_fp), SEEK_SET);

	//입력한 라인 건너뛰기
   	fscanf(file_fp, " %[^\n]", remove_line);

	//다른 명령어 앞으로 당기기
	while(fgets(buf, BUFLEN, file_fp) != NULL)
		fputs(buf, ftmp);

	//명령어 당긴 이후 파일 뒷부분 자르기
	truncate(crontabFile, ftell(ftmp));

	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	fclose(file_fp);
	fclose(ftmp);

	//remove 로그 기록
	write_log(remove_line, "remove");
}

int check_remove_cmd(char *cmd) //인자 검사 오류시 false리턴
{
	int i;
	
	//인자가 없으면 에러
	if(cmd == NULL)
		return false;

	//숫자만 있어야 함
	for(i = 0; i < strlen(cmd); i++)
		if(!isdigit(cmd[i]))
			return false;

	//명령어 번호 초과 x
	if(!(0 <= atoi(cmd) && atoi(cmd) < cmd_count)) 
		return false;

	return true;
}

void print_help() //TODO:usage 출력
{
	printf("도움!!\n");
}
