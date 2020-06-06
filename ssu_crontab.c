#include "default.h"
#include "ssu_crontab.h"

FILE *file_fp; //ssu_crontab_file
FILE *log_fp; //ssu_crontab_log
char crontabFile[FILELEN]; //ssu_crontab_file 경로
char logFile[FILELEN]; //ssu_crontab_log 경로

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
		scanf(" %[^\n]",command);

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
    int fd, i = 0;

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
		fprintf(stdout, "%d. %s\n", i++, buf);

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

int check_add_cmd(char *cmd) //TODO:인자 검사 오류시 false리턴
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

}

void print_help() //TODO:usage 출력
{

}
