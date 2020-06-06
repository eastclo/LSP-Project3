#ifndef SSU_CRONTAB_H_
#define SSU_CRONTAB_H_

void print_prompt(); //프롬프트 출력
int execute_command(char *command); //명령어 구분하여 각각의 명령어 실행함수 호출, exit시 -1리턴

void cmd_add(char *cmd); //add명령어 실행
int check_add_cmd(char *cmd); //인자 검사 오류시 false리턴
void write_log(char *msg, char *flag); //현재 시간, flag(run, add) msg를 로그에 기록

void cmd_remove(char *cmd); //remove 명령어 실행
int check_remove_cmd(char *cmd); //인자 검사 오류시 false리턴

void print_help(); //usage 출력

#endif
