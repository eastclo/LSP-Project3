#include "default.h"
#include "ssu_crond.h"

FILE *file_fp; //ssu_crontab_file
FILE *log_fp; //ssu_crontab_log
char crontabFile[FILELEN]; //ssu_crontab_file 경로
char logFile[FILELEN]; //ssu_crontab_log 경로
Llist* list; //ssu_crontab_file 라인단위로 저장하는 연결리스트

int main(void)
{
	time_t now;
	char buf[FILELEN];
	int fd;

	list = (Llist*)malloc(sizeof(Llist));
	list->head = NULL;
	list->tail = NULL;

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

	//TODO:시간을 x시 x분 00초에 맞춤
	while (1) {
		//매 분마다 명령어 실행
		execute_crontab_cmd(now);
		now += 60;
		sleep(60);
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

	list->cur = list->head;

	//파일 끝에 도달할 때까지 라인단위로 읽음
	while(fscanf(file_fp, " %[^\n]", buf) != EOF) {
		//리스트를 갱신시킴
		if(list->cur == NULL) //tracking 리스트가 없다면 추가
			add_list(buf); //리스트에 추가하고 cur로 만듦
		else if(strcmp(list->cur->cmd.registered, buf) != 0) { //현재 리스트랑 다를 때
			if(search_data(buf))	//이후 리스트에 있다면 그 사이 리스트는 삭제된 것
				delete_from_cur(buf); //리스트에서 삭제하고 buf를 cur로 만듦
			else //이후 리스트에 없다면 새로 추가된 것
				add_list(buf); //리스트에 추가하고 cur로 만듦
		}

		//갱신된 리스트를 실행시킴
		if(check_time(now, list->cur->cmd.registered)) { //실행할 시간이라면
			list->cur->cmd.latest = now; //실행시간 갱신
			system(list->cur->cmd.command); //실행
			write_log(logFile, now, list->cur->cmd.registered); //now시간, run msg 로그 기록
		}

		list->cur = list->cur->next;
	}
	//리스트에 남은 것들 삭제
	delete_remained();

	//리스트 element를 인자로 phtread_create
	
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	fclose(file_fp);
}

void add_list(char *buf) //cur에 새로 buf노드 추가
{
	node *tmp;
	char *ptr;
	int i;

	tmp = (node*)calloc(1, sizeof(node));	
	//node buf정보로 초기화
	strcpy(tmp->cmd.registered, buf);

	ptr = strtok(buf, " "); 
	for(i = 0; i < 5; i++) 
		ptr = strtok(NULL, " "); 
	strcpy(tmp->cmd.command, ptr);

	tmp->cmd.latest = 0;

	//리스트에 아무것도 없을 시
	if(list->head == NULL) {
		list->head = tmp;
		list->cur = tmp;
		list->tail = tmp;
	}
	//tail에 추가할 때
	else if(list->cur == NULL) {
		list->cur = tmp;
		list->cur->prev = list->tail;
		list->tail->next = list->cur;
		list->tail = list->cur;
	}
	//head에 추가할 때
	else if(list->head == list->cur) {
		list->cur = tmp;
		list->cur->next = list->head;
		list->head->prev = list->cur;
		list->head = list->cur;
	}
	//중간 삽입
	else {
		list->cur->prev->next = tmp;
		tmp->prev = list->cur->prev;
		list->cur->prev = tmp;
		tmp->next = list->cur;
		list->cur = tmp;
	}
}

int search_data(char *buf)  //cur이후 registered에 buf가 있다면 true
{
	while(list->cur != NULL) {
		if(strcmp(list->cur->cmd.registered, buf) == 0)
			return true;
		list->cur = list->cur->next;
	}
	return false;
}

void delete_from_cur(char *buf) //cur부터 buf전까지 리스트에서 삭제
{
	node *tmp;
	//cur가 head일 경우
	if(list->cur == list->head) {
		while(strcmp(list->cur->cmd.registered, buf) != 0) {
			tmp = list->cur;	
			list->cur = list->cur->next;
			free(tmp);
		}
		list->head = list->cur;
		list->cur->prev = NULL;
	}
	else {
		tmp = list->cur->prev;
		while(strcmp(list->cur->cmd.registered, buf) != 0) {
			list->cur = list->cur->next;
			list->cur->prev = tmp;
			free(tmp->next);
			tmp->next = list->cur;
		}
	}
}

int check_time(time_t now, char *cmd) //cmd가 실행할 시간이면 true, 아니면 false
{
		
}

void write_log(char *logFile, time_t now, char *msg) //now시간 run msg를 로그에 기록
{
	
}

void delete_remained() //cur부터 tail까지 리스트에 남은 것들 삭제
{
	node *tmp;

	if(list->cur == NULL)
		return;
	//리스트에 cur만 있을 경우
	else if (list ->cur->prev == NULL) {
		list->tail = NULL;
		list->head = NULL;
	}
	else 
		list->tail = list->cur->prev;

	while (list->cur != NULL) {
		tmp = list->cur;
		list->cur = list->cur->next;
		free(tmp);
	}
	if(list->tail != NULL)
		list->tail->next = NULL;
}
