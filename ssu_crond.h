#ifndef SSU_CROND_H_
#define SSU_CROND_H_

#ifndef MIN
	#define MIN 10
#endif
#ifndef HOUR
	#define HOUR 11
#endif
#ifndef DAY
	#define DAY 12
#endif
#ifndef MONTH
	#define MONTH 13
#endif
#ifndef WEEK
	#define WEEK 14
#endif

int daemon_init(void); //디몬프로세스 생성
void execute_crontab_cmd(time_t now); //ssu_crontab_file에서 now시간에 실행할 명령어 실행

int check_times(time_t now, char *cmd); //cmd가 실행할 시간이면 true, 아니면 false
int check_time(int time_val, int flag, char *cycle);//time_val이 flag(분,시,일,월,주)에 따라 cycle에 해당하면 true
void execute_cmd(char *buf); //buf에서 cmd부분 실행
void write_run_log(time_t now, char *msg); //now시간 run msg를 로그에 기록 

#endif
