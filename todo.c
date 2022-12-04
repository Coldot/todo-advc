#pragma warning(disable : 4996)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ISWIN 0                 // 운영체제에 맞게 설정 (윈도우면 0을 1로 변경)
#if ISWIN == 0
    #include <sys/stat.h>
#elif ISWIN == 1
    #include <direct.h>
#endif

#define MAX_INPUT 1000
#define MAX_NAME 90                                     // 한글 기준 최대 30자
#define MAX_LINE 10 + 2 + 5 + 3 + 3 + 1 + MAX_NAME + 10 // = 124
#define PATH_TODO "./todos.txt"                         // TODO DATA 파일 경로
#define PATH_TODO_TEMP "./todos.tmp"                    // 임시 저장용 파일 경로

/* NOTE: todos.txt 형식 예제 (여러 파일로 관리하게 되면 파일 리스트를 가져오는 함수가 환경에 따라 달라져 복잡해져서 우선 하나의 파일에서 전부 동작하도록 단순화해 작업했습니다.)
id:done:due_year:due_month:due_day:name (':'를 구분자로 인식)
1:0:2022:12:16:고급C 최종 보고서 제출
*/

/****** 구조체 ******/
// DATE _ 날짜 정보를 표현하는 자료형
typedef struct _date {
    int year;
    int month;
    int day;

} DATE;

// TODO _ 투두 요소에 대한 자료형
typedef struct _todo {
    int id;
    char done;           // 완료 여부 (0:미완료, 1:완료)
    char name[MAX_NAME]; // 이름
    DATE dueDate;        // 마감일

    // DATE assignedDate;      // 지정일
    // char priority;          // 우선 순위 (0~3, 0:미지정, 1:1순위, 2:2순위, 3:3순위) _ 추후 구현

} TODO;

/****** 폴더 관련 함수 ******/
// 폴더 생성 함수
int createDir(char *name) {
    int result;
    #if ISWIN == 0
        result = mkdir(name, 775);
    #elif ISWIN == 1
        result = mkdir(name);
    #endif
    return result;
}

/****** 데이터 파싱 함수 ******/
// String(char *) -> DATE
// "2022-12-01" 형식으로 들어오는 rawString을 '-' 문자를 기준으로 잘라 DATE 구조체에 담아 반환
DATE parseDate(char *rawString) {
    char raw[MAX_INPUT];
    char *p = NULL;
    DATE date = {};

    strcpy(raw, rawString);

    p = strtok(raw, "-");
    date.year = atoi(p);

    p = strtok(NULL, "-");
    date.month = atoi(p);

    p = strtok(NULL, "-");
    date.day = atoi(p);

    p = strtok(NULL, "-");
    return date;
}

// String(char *) -> TODO
// "1:0:2022:12:16:고급C 최종 보고서 제출" 형식으로 들어오는 rawString을 ':' 문자를 기준으로 잘라 TODO 구조체에 담아 변환
TODO parseTodo(char *rawString) {
    char raw[MAX_LINE];
    char *p = NULL;
    TODO todo = {};

    strcpy(raw, rawString);

    p = strtok(raw, ":");
    todo.id = atoi(p);

    p = strtok(NULL, ":");
    todo.done = (char)atoi(p);

    p = strtok(NULL, ":");
    todo.dueDate.year = atoi(p);

    p = strtok(NULL, ":");
    todo.dueDate.month = atoi(p);

    p = strtok(NULL, ":");
    todo.dueDate.day = atoi(p);

    p = strtok(NULL, ":");
    strcpy(todo.name, p);

    p = strtok(NULL, ":");
    return todo;
}

// String(char *) -> int
// "1:0:2022:12:16:고급C 최종 보고서 제출" 형식으로 오는 데이터에서 id값(예제에서는 1)만 파싱하여 반환
int parseId(char *rawString) {
    char raw[MAX_LINE];
    char *p;
    int id;

    strcpy(raw, rawString);

    p = strtok(raw, ":");
    id = atoi(p);

    return id;
}

// TODO -> String(char *)
char *getTodoString(TODO todo, char *dest) {
    char todoString[MAX_LINE];

    sprintf(todoString, "%d:%d:%d:%d:%d:%s\n",
            todo.id,
            todo.done,
            todo.dueDate.year,
            todo.dueDate.month,
            todo.dueDate.day,
            todo.name);

    strcpy(dest, todoString);
    return dest;
}

/****** 필요한 데이터 계산/처리(프로세싱) 함수 ******/
// 새로 만들 TODO의 id값을 구하는 함수
// TODO 데이터 파일을 읽어 가장 마지막 id보다 더 큰 값을 반환
int getNewId(char *path) {
    char raw_line[MAX_LINE];
    int newId = 0;

    FILE *fp = fopen(path, "r");

    // NOTE: TODO 데이터 파일이 존재하지 않을 경우 id는 첫 번째 id인 1이 되어야 함
    if (fp == NULL)
        return 1;

    while (fgets(raw_line, MAX_LINE, fp) != NULL) {
        raw_line[strlen(raw_line) - 1] = '\0';

        newId = parseId(raw_line);
    }
    newId += 1;

    fclose(fp);
    return newId;
}

// 파일의 총 열을 구하는 함수
int getRow(char *path) {
    char raw_line[MAX_LINE];
    int row = 0;

    FILE *fp = fopen(path, "r");

    if (fp == NULL)
        return 0;
    
    while (fgets(raw_line, MAX_LINE, fp) != NULL) {
        raw_line[strlen(raw_line) - 1] = '\0';

        if (strlen(raw_line) != 0)
            row++;
    }

    fclose(fp);
    return row;
}

/****** TODO 관리 함수들 ******/
// 새로운 TODO 추가
// 인자로 들어온 데이터를 기반으로 새로운 TODO를 만들어 파일에 바로 저장하고 TODO 반환
TODO writeNewTodo(char *name, DATE due) {
    int id;
    TODO new = {};

    id = getNewId(PATH_TODO);
    FILE *fp = fopen(PATH_TODO, "a+");

    new.id = id;
    new.done = 0;
    strcpy(new.name, name);
    new.dueDate = due;

    fprintf(fp, "%d:%d:%d:%d:%d:%s\n",
            new.id,
            new.done,
            new.dueDate.year,
            new.dueDate.month,
            new.dueDate.day,
            new.name);

    fclose(fp);
    return new;
}

// 기존 TODO 삭제
// 고유한 TODO의 id 값을 기준으로 삭제 (파일에서도 실시간 반영)
// 조회 시 id 값을 같이 보여주고 id값을 입력받아 삭제하도록 구현
void removeTodo(int targetId) {
    char raw_line[MAX_LINE];
    int currId;

    FILE *fp = fopen(PATH_TODO, "r");
    FILE *fp_temp = fopen(PATH_TODO_TEMP, "w");

    while (fgets(raw_line, MAX_LINE, fp) != NULL) {
        // 삭제할 때에는 문자열의 끝에 개행문자를 남겨둠

        currId = parseId(raw_line);

        // 삭제 대상의 line은 건너띄고 새로운 파일에 기록
        if (currId == targetId)
            continue;

        fputs(raw_line, fp_temp);
    }

    fclose(fp);
    fclose(fp_temp);

    remove(PATH_TODO);
    rename(PATH_TODO_TEMP, PATH_TODO);
}

// 기존 TODO 이름 변경
void renameTodo(int targetId) {  
    char raw_line[MAX_LINE];
    int currId;
    char sen[MAX_NAME] = "";

    FILE *fp = fopen(PATH_TODO, "r");
    FILE *fp_temp = fopen(PATH_TODO_TEMP, "w");

    while (fgets(raw_line, MAX_LINE, fp) != NULL) {
        // 이름 변경시에는 개행 문자 남겨둬도 무관

        currId = parseId(raw_line);
        TODO todo = parseTodo(raw_line);

        // 삭제 대상의 line은 건너띄고 새로운 파일에 기록
        if (currId == targetId) {
            printf("수정 할 문장을 입력해주세요 : ");
            scanf("%[^\n]s", sen);
            getchar();
            strcpy(todo.name, sen);
            getTodoString(todo, raw_line);
        }

        fputs(raw_line, fp_temp);
    }

    fclose(fp);
    fclose(fp_temp);

    remove(PATH_TODO);
    rename(PATH_TODO_TEMP, PATH_TODO);
}

// TODO 보여주기
void showTodo()
{
    int numOfRows = getRow(PATH_TODO);
    char raw_line[MAX_LINE];

    FILE* fp = fopen(PATH_TODO, "r");

    if (fp == NULL || numOfRows == 0) {
        printf("TODO가 존재하지 않습니다.");
        return;
    } 

    printf("완료 \t id \t 마감일 \t\t 할일\n");
    while (fgets(raw_line, MAX_LINE, fp) != NULL) {   
        raw_line[strlen(raw_line) - 1] = '\0';

        TODO todo = parseTodo(raw_line);
        printf("[%s] \t %d \t\t %04d-%02d-%02d \t %s\n", 
            todo.done ? "✓" : " ", 
            todo.id,
            todo.dueDate.year, todo.dueDate.month, todo.dueDate.day,
            todo.name);
    }
    printf("==================================\n");

    fclose(fp);
    return;

}

// TODO 완료 체크하기
void toggleTodo(int targetId) {
    char raw_line[MAX_LINE];
    int currId;

    FILE *fp = fopen(PATH_TODO, "r");
    FILE *fp_temp = fopen(PATH_TODO_TEMP, "w");

    while (fgets(raw_line, MAX_LINE, fp) != NULL) {
        currId = parseId(raw_line);
        TODO todo = parseTodo(raw_line);
        todo.name[strlen(todo.name) - 1] = '\0';

        if (currId == targetId) {
            
            if ( !todo.done ) {
                todo.done = 1;
                printf("완료 처리하였습니다.\n");
            }
            else {
                char input[10];
                printf("이미 완료 처리된 Todo 입니다. 체크를 해제할까요? [y/n]: ");
                scanf("%s", input);
                getchar();

                if (strcmp(input, "y") == 0) {
                    todo.done = 0;
                    printf("체크를 해제하였습니다.\n");
                } else if (strcmp(input, "n") == 0) {
                    printf("체크를 해제하지 않습니다.\n");
                } else {
                    printf("알 수 없는 답변입니다. 체크를 해제하지 않습니다.\n");
                }
            }

            getTodoString(todo, raw_line);
        }

        fputs(raw_line, fp_temp);
    }

    fclose(fp);
    fclose(fp_temp);

    remove(PATH_TODO);
    rename(PATH_TODO_TEMP, PATH_TODO);
}

/****** 각 메뉴별 함수 ******/
// 메뉴 1. TODO 추가
void menu_newTodo() {
    TODO todo = {0, 0, "", {0, 0, 0}};
    char input_name[MAX_NAME] = "";
    char input[MAX_INPUT] = "";

    printf("\n");
    printf("=== 1. TODO 추가 ===\n");

    printf("TODO 이름: ");
    scanf("%[^\n]s", input_name);
    getchar();
    strcpy(todo.name, input_name);

    printf("마감일(예: 1970-01-01): ");
    scanf("%s", input);
    getchar();
    todo.dueDate = parseDate(input);

    todo = writeNewTodo(todo.name, todo.dueDate);
    printf("저장을 완료하였습니다. (id: %d)\n", todo.id);
    printf("===================\n");

    return;
}

// 메뉴 2. TODO 삭제
void menu_removeTodo() {
    int id = 0;

    printf("\n");
    printf("=== 2. TODO 삭제 ===\n");
    printf("삭제할 TODO ID: ");
    scanf("%d", &id);
    getchar();

    removeTodo(id);
    printf("삭제를 완료하였습니다.\n");
    printf("===================\n");

    return;
}

// 메뉴 3. TODO 수정
void menu_renameTodo() {
    int targetId;

    printf("수정할 todo의 id를 입력하세요: ");
    scanf("%d", &targetId);
    getchar();

    renameTodo(targetId);
}

// 메뉴 4. TODO 보여주기
void menu_showTodo() {
	showTodo();
	
} 

// 메뉴 5. TODO 완료 체크
void menu_doneTodo() {
    int id = 0;

    printf("\n");
    printf("=== 5. TODO 완료 체크 ===\n");
    printf("완료할 TODO ID: ");
    scanf("%d", &id);
    getchar();
    toggleTodo(id);
    printf("===================\n");

    return;
}

// 메뉴 11. 초기 설정
// 프로그램이 정상 작동하려면 data 폴더가 존재하여야 하므로, 최초 실행 시 1회 실행 필요
// void menu_initDataDir() {
//     if (createDir(DIRNAME) == 0) {
//         printf("초기 설정 완료\n");
//     }
//     else {
//         printf("초기 설정 실패\n");
//     }
// }

/****** 화면 초기화 함수 ******/
void clear() {
    #if ISWIN == 0
    system("clear");
    #elif ISWIN == 1
    system("cls");
    #endif
}


/****** main ******/
int main() {
    int cmd = 0;

    while (1) {
        printf("\n\n");
        printf("============\n");
        printf("0. 종료\n");
        printf("------------\n");
        printf("1. TODO 추가\n");
        printf("2. TODO 삭제\n");
        printf("3. TODO 수정\n");
        printf("4. TODO 보기\n");
        printf("5. TODO 완료 토글\n");
        // printf("============\n");
        // printf("11. 초기 설정 (최초 1회 실행 필요)\n");
        printf("============\n");
        printf("> ");
        scanf("%d", &cmd);
        getchar();

        // 0. 종료
        if (cmd == 0) {
            printf("프로그램을 종료합니다.\n");
            break;
        }

        clear();

        switch (cmd) {
        // 1. TODO 추가
        case 1:
            menu_newTodo();
            break;

        // 2. TODO 삭제
        case 2:
            menu_removeTodo();
            break;

        // 3. TODO 수정
        case 3:
            menu_renameTodo();
            break;
        
        // 4. TODO 보기
        case 4:
            menu_showTodo();
            break;
        
        // 5. TODO 체크
        case 5:
            menu_doneTodo();
            break;

        // 11. 초기 설정 (사용x)
        // case 11:
        //     menu_initDataDir();
        //     break;

        default:
            printf("존재하지 않는 메뉴입니다.\n");
            break;
        }
    }

    return 0;
}
