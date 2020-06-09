#include "default.h" 
#include "ssu_rsync.h"

struct timeval begin_t, end_t;
char *my_extension = ".zxcv_swq"; //동기화 중 기존 파일 구분자
char src_fname[FILELEN]; //src name
char src_parent[FILELEN]; //src parent path
char dst_rpath[FILELEN]; //dst realpath
char logFile[FILELEN] = "ssu_rsync_log";
int rOption = false;
int tOption = false;
int mOption = false;
char sync_files[100][FILELEN];
int sync_count = 0;

int main(int argc, char *argv[])
{
	gettimeofday(&begin_t, NULL); //프로그램 시작시간 측정

	if(atexit(ssu_runtime)) {
		fprintf(stderr, "atexit error\n");
		exit(1);
	}
	ssu_rsync(argc, argv);

	exit(0);
}

void ssu_runtime(void) //프로그램 수행시간 출력함수
{
	gettimeofday(&end_t, NULL); //프로그램 종료시간 측정
    end_t.tv_sec -= begin_t.tv_sec;  //시작시간 초와 종료시간초의 차이 계산

    if(end_t.tv_usec < begin_t.tv_usec){  //시작시과 종료시간 micro초차이 계산
        end_t.tv_sec--;
        end_t.tv_usec += SECOND_TO_MICRO;
    }

    end_t.tv_usec -= begin_t.tv_usec;
    printf("Runtime: %ld:%06ld(sec:usec)\n", end_t.tv_sec % 60, end_t.tv_usec);
}

void ssu_rsync(int argc, char **argv) //동기화 시작
{
	void (*ssu_func)(int);
	struct dirent *dirp;
	struct stat statbuf;
	char *ptr;
	DIR *dp;

	//인자 오류시 usage 출력 후 종료
	if(check_argument(argc, argv) == false) {
		print_help();
		return;
	}

	ssu_func = signal(SIGINT, sigint_handler);

	lstat(argv[1], &statbuf);
	//src가 디렉토리일 경우
	if(S_ISDIR(statbuf.st_mode)) {
		dp = opendir(src_parent);
		while((dirp = readdir(dp)) != NULL) {
			if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
				continue;
			lstat(dirp->d_name, &statbuf);
			//서브 디렉토리는 r옵션 있을 때만
			if(S_ISDIR(statbuf.st_mode) && rOption) 
				sync_file(dirp->d_name); //디렉토리 이름
			else 
				sync_file(dirp->d_name);
		}
	}
	//src가 일반 파일일 경우
	else 
		sync_file(src_fname); //파일 이름을 src에서 dst로 동기화 

	//SIGINT default 핸들러로 변경
	signal(SIGINT, ssu_func);

	write_log(argc, argv); //로그작성
	delete_save_file(); //동기화 취소를 대비한 세이브파일 삭제
}

int check_argument(int argc, char **argv) //TODO:인자 오류시 false리턴
{
	struct stat statbuf;
	char *ptr;

	if(realpath(argv[1], src_parent) == NULL) {
		fprintf(stderr, "realpath error\n");
		return false;
	}
	if(realpath(argv[2], dst_rpath) == NULL) {
		fprintf(stderr, "realpath error\n");
		return false;
	}


	/*예외처리 끝난 후 src가 파일, 디렉토리일 경우 나눠서 경로 저장*/
	lstat(src_parent, &statbuf);
	
	//디렉토리일 경우
	if(S_ISDIR(statbuf.st_mode))
		return true;
	//파일일 경우
	else {
		//src 부모 디렉토리 경로 저장
		ptr = src_parent + strlen(src_parent) - 1;
		while(*ptr != '/')
			*(ptr--) = 0;
		*ptr = 0;

		//src파일명 복사
		strcpy(src_fname, argv[1]);
	}
}

void sync_file(char *fname) //해당 파일을 src에서 dst로 동기화
{
	struct stat statbuf;
	char obj_path[FILELEN]; //동기화 대상 파일명(src)
	char synced_path[FILELEN]; //동기화 된 파일명(dst폴더)
	char tmp[FILELEN];
	
	sprintf(obj_path, "%s/%s", src_parent, fname);
	sprintf(synced_path, "%s/%s", dst_rpath, fname);

	if(check_same_file(obj_path, synced_path) == false) //동일한 파일이 있으면 동기화x
		return;

	lstat(obj_path, &statbuf);
	//r옵션에서 디렉토리 이름을 넘겨준 것
	if(S_ISDIR(statbuf.st_mode)) {

	}
	//일반적인 상황
	else {
		if(access(synced_path, F_OK) == 0) {
			//동일한 파일명이 존재하면 임의 확장자 붙여서 이름변경
			sprintf(tmp, "%s%s", synced_path, my_extension);	
			rename(synced_path, tmp); 
		}
		//synced_path로 복사하고 수정시간도 동일하게 변경
		copy_file(obj_path, synced_path);
	}
}

void sigint_handler(int signo) //SIGINT시 동기화 취소 후 종료
{
	struct dirent *dirp;
    DIR *dp;
	char original[BUFLEN];
	char tmp[BUFLEN];
	char *ptr;
	int i;

	//sync_files에서 sync_count만큼 파일 삭제
	for(i = 0; i < sync_count; i++)
		unlink(sync_files[i]);
	
	//my_extension 붙은 파일들 확장자 제거
    dp = opendir(dst_rpath);
    while((dirp = readdir(dp)) != NULL) {
        if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
            continue;
		
		//save파일이면 확장자 제거
		if(strstr(dirp->d_name, my_extension) != NULL) {
			sprintf(original, "%s/%s", dst_rpath, dirp->d_name);
			sprintf(tmp, "%s", original);
			ptr = tmp + strlen(tmp);
			while (*(--ptr) != '.');
			*ptr = 0;
			rename(original, tmp);
		}
	}
	printf("동기화 취소\n");

	exit(1);
}

int check_same_file(char *fold, char *fnew) //fnew 파일이 이미 존재하고 fold와 수정시간이 같으면 false
{
	struct stat new_stat;
	struct stat old_stat;
	if(access(fnew, F_OK) == 0) {
		lstat(fnew, &new_stat);
		lstat(fold, &old_stat);
		if(old_stat.st_mtime == new_stat.st_mtime)
			return false;
	}

	return true;
}

void copy_file(char *oldFile, char *newFile) //oldfile을 newfile명으로 수정정시간도 동일하게 복사
{
	struct utimbuf time_buf;
	struct stat statbuf;
	FILE *fold, *fnew;
	char buf[BUFLEN];

	fold = fopen(oldFile, "r");
	fnew = fopen(newFile, "w");
		
	if(fold == NULL || fnew == NULL) {
		fprintf(stderr, "fopen error %s, %s\n", oldFile, newFile);
		return;
	}

	//파일 내용 복사
	while(fgets(buf, BUFLEN, fold) != NULL) 
		fprintf(fnew, "%s", buf);

	//동기화한 파일명 저장
	strcpy(sync_files[sync_count++], newFile);

	fclose(fold);
	fclose(fnew);

	if(lstat(oldFile, &statbuf) < 0) {
		fprintf(stderr, "lstat error for %s\n", oldFile);
		return;
	}

	//수정시간 변경
	time_buf.modtime = statbuf.st_mtime;
	if(utime(newFile, &time_buf) < 0) {
		fprintf(stderr, "utime error for %s\n", newFile);
		return;
	}
}

void write_log(int argc, char **argv) //동기화 완료 로그 기록
{
	struct stat statbuf;
	struct tm nowTime;
	FILE *log_fp;
	char *time_ptr;
	char *ptr;
	time_t now;
	int i;

	if(access(logFile, F_OK) < 0) {
		log_fp = fopen(logFile, "w");
		fclose(log_fp);
	}

	log_fp = fopen(logFile, "r+");

	now = time(NULL);

	fseek(log_fp, 0, SEEK_END);
	//시간, 명령어 기록
	time_ptr = ctime(&now);
	time_ptr[strlen(time_ptr)-1] = ']';
	fprintf(log_fp, "[%s", time_ptr);
	for(i = 0; i < argc; i++)
		fprintf(log_fp, " %s", argv[i]);
	fprintf(log_fp, "\n");

	//동기화 한 파일과 크기 작성
	for(i = 0; i < sync_count; i++) {
		lstat(sync_files[i], &statbuf);

		ptr = sync_files[i] + strlen(sync_files[i]);	
		while(*(--ptr) != '/');

		fprintf(log_fp, "\t%s %ldbytes\n", ++ptr, statbuf.st_size);
	}

	fclose(log_fp);
}

void delete_save_file() //동기화 취소를 대비한 save 파일 삭제
{
    struct dirent *dirp;
    DIR *dp;
	char tmp[BUFLEN];

    dp = opendir(dst_rpath);
    while((dirp = readdir(dp)) != NULL) {
        if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
            continue;
		
		//save파일이면 삭제
		if(strstr(dirp->d_name, my_extension) != NULL) {
			sprintf(tmp, "%s/%s", dst_rpath, dirp->d_name);
			unlink(tmp);
		}
	}   
}

void print_help() //TODO:
{
}
