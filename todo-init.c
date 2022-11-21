#include <stdio.h>

#define ISWIN == 0
#if ISWIN == 0
	#include <sys/stat.h>
#elif ISWIN == 1
	#include <direct.h>
#endif

#define DIRNAME "./data"


int createDir(char *name) {
	int result;
	#if ISWIN == 0
		result = mkdir(name, 775);
	#elif ISWIN == 1
		result = mkdir(name);
	#endif
}


int main() {
	if (createDir(DIRNAME) == 0) {
		printf("초기 설정 완료\n");
	}
	else {
		printf("초기 설정 실패\n");
	}
	
	return 0;
}
