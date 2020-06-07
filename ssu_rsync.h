#ifndef SSU_RSYNC_H_
#define SSU_RSYNC_H_

void ssu_runtime(void); //프로그램 수행시간 출력함수

void ssu_rsync(int argc, char **argv); //동기화 시작
int check_argument(int argc, char **argv); //인자 오류시 false리턴

void sync_file(char *fname); //해당 파일을 src에서 dst로 동기화

void sigint_handler(int signo); //SIGINT시 동기화 취소 후 종료
int check_same_file(char *fold, char *fnew); //fnew 파일이 이미 존재하고 fold와 수정시간이 같으면 fals
void copy_file(char *oldFile, char *newFile); //oldfile을 newfile명으로 수정시간도 동일하게 복사

void write_log(int argc, char **argv); //동기화 완료 로그 기록
void delete_save_file(); //동기화 취소를 대비한 save 파일 삭제

void print_help(); //usage 출력

#endif
