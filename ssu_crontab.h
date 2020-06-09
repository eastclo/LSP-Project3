#ifndef SSU_CRONTAB_H_
#define SSU_CRONTAB_H_

#ifndef STAR 
	#define STAR -1 
#endif
#ifndef BAR 
	#define BAR -2 
#endif
#ifndef DIVIDE
	#define DIVIDE -3 
#endif
#ifndef COMMA
	#define DIVIDE -4 
#endif

void ssu_runtime(void); //프로그램 수행시간 출력함수

void print_prompt(); //프롬프트 출력
int execute_command(char *command); //명령어 구분하여 각각의 명령어 실행함수 호출, exit시 -1리턴

void cmd_add(char *cmd); //add명령어 실행
int check_add_cmd(char *cmd); //인자 검사 오류시 false리턴
int check_cycle(int low, int high, char *cycle); //cycle이 low, high에 존재하고, 형식에 이상 없으면 true
void write_log(char *msg, char *flag); //현재 시간, flag(run, add) msg를 로그에 기록

void cmd_remove(char *cmd); //remove 명령어 실행
int check_remove_cmd(char *cmd); //인자 검사 오류시 false리턴

void print_help(); //usage 출력

#endif
