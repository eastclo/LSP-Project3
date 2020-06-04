#ifndef SSU_CROND_H_
#define SSU_CROND_H_

struct crontab_command{ //예약 명령어
    char registered[BUFLEN]; //crontab에 등록된 실행주기와 명령어
    char command[BUFLEN]; //registerd에서 명령어 부분
    time_t latest; //가장 최근에 실행한 시간
};

typedef struct Llist { //연결리스트
	struct node *head;
	struct node *cur;
	struct node *tail;
} Llist;

typedef struct node { //연결리스트 element
	struct node *next;
	struct node *prev;
	struct crontab_command cmd;
} node;

int daemon_init(void); //디몬프로세스 생성
void execute_crontab_cmd(time_t now); //ssu_crontab_file에서 now시간에 실행할 명령어 실행

void add_list(char *buf); //cur에 새로 buf노드 추가
int check_time(time_t now, char *cmd); //cmd가 실행할 시간이면 true, 아니면 false
int search_data(char *buf); //cur이후 registerd에 buf가 있다면 true
void delete_from_cur(char *buf); //cur부터 buf전까지 리스트에서 삭제

void write_run_log(char *logFile, time_t now, char *msg); //now시간 run msg를 로그에 기록 
void delete_remained(); //cur부터 tail까지 리스트에 남은 것들 삭제

#endif
